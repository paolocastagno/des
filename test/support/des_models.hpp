#ifndef DES_MODELS_HPP
#define DES_MODELS_HPP

// Model builders shared by the test suites and the benchmarks.

#include <climits>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_fifo.hpp>
#include <libdes_is.hpp>
#include <libdes_network.hpp>
#include <libdes_ps.hpp>
#include <libdes_scalar.hpp>
#include <libdes_sink.hpp>
#include <libdes_source.hpp>
#include <libdes_station.hpp>

namespace des_models
{
	using exp_station = des::station<double, std::exponential_distribution>;
	using exp_dists = std::vector<std::vector<std::shared_ptr<std::exponential_distribution<double>>>>;
	/** Station with deterministic service times, see fixed(). */
	using det_station = des::station<double, std::uniform_real_distribution>;
	using det_dists = std::vector<std::vector<std::shared_ptr<std::uniform_real_distribution<double>>>>;
	using routing_t = std::vector<std::vector<std::vector<double>>>;

	constexpr unsigned int UNLIMITED = std::numeric_limits<unsigned int>::max();

	inline std::shared_ptr<std::mt19937_64> engine(unsigned long long seed)
	{
		return std::make_shared<std::mt19937_64>(seed);
	}

	/** Exponential distribution with rate @p rate. */
	inline std::shared_ptr<std::exponential_distribution<double>> expo(double rate)
	{
		return std::make_shared<std::exponential_distribution<double>>(rate);
	}

	/** Distribution always returning @p v (the station template is instantiated for <random> distributions only). */
	inline std::shared_ptr<std::uniform_real_distribution<double>> fixed(double v)
	{
		return std::make_shared<std::uniform_real_distribution<double>>(v, v);
	}

	/** Event of class @p cls at time @p t, owned by node @p node. */
	inline std::shared_ptr<des::event> make_event(int cls, double t, int node = 0)
	{
		auto e = std::make_shared<des::event>(cls);
		e -> set_time(t);
		e -> set_info(EVENT_NODE, node);
		return e;
	}

	/** Delivers a new event of class @p cls at time @p t to node @p n; returns it (nullptr if refused). */
	inline std::shared_ptr<des::event> arrive(des::node& n, int cls, double t, int node = 0)
	{
		auto e = make_event(cls, t, node);
		return n.arrival(e) ? e : nullptr;
	}

	/** Routing where every class of node i goes to node next[i] (-1: leaves the network). */
	inline routing_t chain(const std::vector<int>& next, unsigned int classes = 1)
	{
		size_t n = next.size();
		routing_t r(n, std::vector<std::vector<double>>(n, std::vector<double>(classes, 0.0)));
		for(size_t i = 0; i < n; i++)
			if(next[i] >= 0)
				for(unsigned int c = 0; c < classes; c++)
					r[i][next[i]][c] = 1.0;
		return r;
	}

	/**
	 * Open network: source (node 0) -> stations (nodes 1..k, in series) -> sink (node k+1).
	 * The first station's sojourn time is observed per class.
	 */
	struct open_model
	{
		std::shared_ptr<std::mt19937_64> gen;
		std::shared_ptr<des::source> src;
		std::vector<std::shared_ptr<des::node>> stations;
		std::shared_ptr<des::sink> snk;
		std::vector<std::shared_ptr<des::scalar>> sojourn;   ///< one per station
		std::unique_ptr<des::network> net;
		double last_time = 0.0;

		/**
		 * @param lambda  arrival rate per class
		 * @param make    builds the stations, given the shared engine
		 */
		open_model(std::vector<double> lambda, unsigned long long seed,
				   const std::function<std::vector<std::shared_ptr<des::node>>(std::shared_ptr<std::mt19937_64>)>& make)
			: gen(engine(seed))
		{
			int classes = static_cast<int>(lambda.size());
			src = std::make_shared<des::source>(lambda, "source", gen);
			stations = make(gen);
			snk = std::make_shared<des::sink>("sink", classes);
			std::vector<std::shared_ptr<des::node>> nodes{src};
			std::vector<int> next{1};
			for(size_t i = 0; i < stations.size(); i++)
			{
				auto s = std::make_shared<des::scalar>(NODE_SOJOURN, classes);
				stations[i] -> attach(SIGNAL_NODE_DEPARTURE, s);
				sojourn.push_back(s);
				nodes.push_back(stations[i]);
				next.push_back(static_cast<int>(i) + 2);
			}
			nodes.push_back(snk);
			next.push_back(-1);
			net = std::make_unique<des::network>(nodes, chain(next, classes), gen);
			for(int c = 0; c < classes; c++)
				src -> arrival(make_event(c, 0.0, 0));
		}

		/** Processes @p events events. */
		void step(int events)
		{
			for(int i = 0; i < events; i++)
			{
				auto e = net -> next_event();
				last_time = e -> get_time();
				net -> route(e);
			}
		}

		/** Discards a warm-up of @p events events, then runs @p runs replications of @p events_per_run events. */
		void run(int warmup, int events_per_run, int runs)
		{
			step(warmup);
			net -> reset(last_time, {}, false);
			for(int r = 0; r < runs; r++)
			{
				step(events_per_run);
				net -> reset(last_time, {}, true);
			}
		}
	};

	/** Single-class M/M/c/K-like station: c servers, one waiting queue of @p places places. */
	inline std::shared_ptr<des::node> mm_station(std::shared_ptr<std::mt19937_64> g, double mu, unsigned int servers = 1, unsigned int places = INT_MAX)
	{
		exp_dists d(servers, {expo(mu)});
		return std::make_shared<exp_station>(d, servers, 1, 1, places, "station", g);
	}
}

#endif
