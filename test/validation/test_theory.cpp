// Validation tier: simulated estimates against queueing theory.
//
// Each check runs independent replications after a warm-up and accepts the model when the
// theoretical value lies in the replications' Student-t confidence interval. Seeds are
// fixed, so every run on a platform gives the same verdict; the 99.9% level keeps the
// chance of a false failure on a new platform (different <random> streams) small.

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_observer.hpp>
#include <libdes_util.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

namespace
{
	constexpr double ALPHA = 0.001;
	constexpr int WARMUP = 20000;
	constexpr int EVENTS = 100000;
	constexpr int RUNS = 10;

	std::vector<std::shared_ptr<des::node>> one(std::shared_ptr<des::node> n)
	{
		return {n};
	}

	/** Checks that theory lies in the interval, noting both. */
	void expect(const std::string& what, double theory, std::pair<double, double> ci)
	{
		NOTE(what << ": theory " << theory << ", " << (1 - ALPHA) * 100 << "% CI [" << ci.first << ", " << ci.second << "]");
		if(!(ci.first <= theory && theory <= ci.second))
			des_test::fail(__FILE__, __LINE__, what + ": theory " + des_test::show(theory) + " outside [" + des_test::show(ci.first) + ", " + des_test::show(ci.second) + "]");
	}

	void expect_sojourn(open_model& m, size_t station, int cls, double theory, const std::string& what)
	{
		expect(what, theory, m.sojourn[station] -> confidence_interval(ALPHA, cls));
	}

	/** Mean sojourn over all classes, one value per completed run. */
	struct pooled_sojourn : des::observer
	{
		double sum = 0;
		long n = 0;
		std::vector<double> runs;
		void update(const des::message& m) override { sum += m.get_value(NODE_SOJOURN); ++n; }
		void reset(bool newrun) override
		{
			if(newrun && n > 0) runs.push_back(sum / n);
			sum = 0;
			n = 0;
		}
		void reset(int, bool) override {}
		void clear() override { sum = 0; n = 0; runs.clear(); }
		std::string to_string() const override { return "pooled_sojourn"; }
	};
}

TEST_CASE("M/M/1: mean sojourn and throughput", "[validation]")
{
	const double lambda = 0.8, mu = 1.0;
	open_model m({lambda}, 201, [&](auto g) { return one(mm_station(g, mu)); });
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, 1 / (mu - lambda), "sojourn");
	expect("throughput", lambda, m.net -> get_flow_ci(1, 2, 0, ALPHA));
}

TEST_CASE("M/M/2: Erlang-C mean sojourn", "[validation]")
{
	const double lambda = 1.6, mu = 1.0, a = lambda / mu, rho = a / 2;
	const double term = a * a / (2 * (1 - rho)), c = term / (1 + a + term);
	open_model m({lambda}, 202, [&](auto g) { return one(mm_station(g, mu, 2)); });
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, c / (2 * mu - lambda) + 1 / mu, "sojourn");
}

TEST_CASE("M/D/1 FCFS: Pollaczek-Khinchine mean sojourn", "[validation]")
{
	const double lambda = 0.8, rho = lambda;
	open_model m({lambda}, 203, [](auto g) { return one(std::make_shared<det_station>(det_dists{{fixed(1.0)}}, 1, 1, 1, INT_MAX, "md1", g)); });
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, 1 + rho / (2 * (1 - rho)), "sojourn");
}

TEST_CASE("M/M/1-PS and M/D/1-PS: mean sojourn E[S]/(1-rho)", "[validation]")
{
	const double lambda = 0.8;
	open_model mm({lambda}, 204, [](auto g)
	{
		return one(std::make_shared<exp_station>(exp_dists{{expo(1.0)}}, 1, UNLIMITED, std::make_shared<des::ps>(), "ps", g));
	});
	mm.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(mm, 0, 0, 1 / (1 - lambda), "M/M/1-PS sojourn");
	open_model md({lambda}, 205, [](auto g)
	{
		return one(std::make_shared<det_station>(det_dists{{fixed(1.0)}}, 1, UNLIMITED, std::make_shared<des::ps>(), "ps", g));
	});
	md.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(md, 0, 0, 1 / (1 - lambda), "M/D/1-PS sojourn (insensitivity)");
}

TEST_CASE("multi-class PS: per-class mean sojourn E[S_c]/(1-rho)", "[validation]")
{
	const double l0 = 0.3, l1 = 0.2, mu0 = 1.0, mu1 = 0.5, rho = l0 / mu0 + l1 / mu1;
	open_model m({l0, l1}, 206, [&](auto g)
	{
		return one(std::make_shared<exp_station>(exp_dists{{expo(mu0), expo(mu1)}}, 1, UNLIMITED, std::make_shared<des::ps>(), "ps", g));
	});
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, (1 / mu0) / (1 - rho), "class 0 sojourn");
	expect_sojourn(m, 0, 1, (1 / mu1) / (1 - rho), "class 1 sojourn");
}

TEST_CASE("weighted PS is work-conserving: overall M/M/1 mean sojourn", "[validation]")
{
	// Same exponential service for both classes: any work-conserving discipline has the
	// M/M/1 number in system, whatever the weights
	const double l0 = 0.4, l1 = 0.4, mu = 1.0;
	auto pooled = std::make_shared<pooled_sojourn>();
	open_model m({l0, l1}, 207, [&](auto g)
	{
		auto sta = std::make_shared<exp_station>(exp_dists{{expo(mu), expo(mu)}}, 1, UNLIMITED, std::make_shared<des::ps>(std::vector<double>{3.0, 1.0}), "gps", g);
		sta -> attach(SIGNAL_NODE_DEPARTURE, pooled);
		return one(sta);
	});
	m.run(WARMUP, EVENTS, RUNS);
	expect("overall sojourn", 1 / (mu - l0 - l1), conf_int(pooled -> runs, ALPHA));
	CHECK(m.sojourn[0] -> get_scalar(0) < m.sojourn[0] -> get_scalar(1));   // class 0 has the larger share
}

TEST_CASE("M/M/inf: mean sojourn 1/mu", "[validation]")
{
	open_model m({50.0}, 208, [](auto g) { return one(std::make_shared<exp_station>(exp_dists{{expo(1.0)}}, 1, UNLIMITED, std::make_shared<des::is>(), "is", g)); });
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, 1.0, "sojourn");
}

TEST_CASE("M/M/2/2: Erlang-B loss probability", "[validation]")
{
	const double lambda = 1.5, mu = 1.0, a = lambda / mu;
	const double erlang_b = (a * a / 2) / (1 + a + a * a / 2);
	open_model m({lambda}, 209, [&](auto g) { return one(mm_station(g, mu, 2, 0)); });
	m.step(WARMUP);
	m.net -> reset(m.last_time, {}, false);
	std::vector<double> loss;
	for(int r = 0; r < RUNS; r++)
	{
		m.step(EVENTS);
		double lost = m.net -> get_lost(0, 0), routed = m.net -> get_count(0, 1, 0);
		loss.push_back(lost / (lost + routed));
		m.net -> reset(m.last_time, {}, true);
	}
	expect("loss probability", erlang_b, conf_int(loss, ALPHA));
}

TEST_CASE("tandem (Jackson): per-station mean sojourn", "[validation]")
{
	const double lambda = 0.8, mu1 = 1.0, mu2 = 1.25;
	open_model m({lambda}, 210, [&](auto g) { return std::vector<std::shared_ptr<des::node>>{mm_station(g, mu1), mm_station(g, mu2)}; });
	m.run(WARMUP, EVENTS, RUNS);
	expect_sojourn(m, 0, 0, 1 / (mu1 - lambda), "station 1 sojourn");
	expect_sojourn(m, 1, 0, 1 / (mu2 - lambda), "station 2 sojourn");
}

TEST_CASE("closed network: throughput from mean value analysis", "[validation]")
{
	const std::vector<double> mu{1.0, 2.0};
	const int population = 3;
	// Exact MVA for a cycle of single-server exponential stations
	std::vector<double> q(mu.size(), 0.0);
	double x = 0;
	for(int n = 1; n <= population; n++)
	{
		std::vector<double> r(mu.size());
		double total = 0;
		for(size_t i = 0; i < mu.size(); i++) { r[i] = (1 + q[i]) / mu[i]; total += r[i]; }
		x = n / total;
		for(size_t i = 0; i < mu.size(); i++) q[i] = x * r[i];
	}
	auto gen = engine(211);
	auto a = mm_station(gen, mu[0]), b = mm_station(gen, mu[1]);
	des::network net({a, b}, chain({1, 0}), gen);
	for(int k = 0; k < population; k++) a -> arrival(make_event(0, 0.0, 0));
	double t = 0;
	auto step = [&](int events) { for(int i = 0; i < events; i++) { auto e = net.next_event(); t = e -> get_time(); net.route(e); } };
	step(WARMUP);
	net.reset(t, {}, false);
	for(int r = 0; r < RUNS; r++)
	{
		step(EVENTS);
		net.reset(t, {}, true);
	}
	expect("throughput", x, net.get_flow_ci(0, 1, 0, ALPHA));
}
