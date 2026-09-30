#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_counter.hpp>
#include <libdes_fifo.hpp>
#include <libdes_node.hpp>
#include <libdes_observer.hpp>
#include <libdes_queue.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

namespace
{
	// Minimal concrete node reaching the (cls, queues, servers, maps, ...) constructor
	struct probe_node : des::node
	{
		using des::node::node;
		double get_service(unsigned int&, unsigned int&) override { return 1.0; }
		int schedule(const std::shared_ptr<des::event>&, const std::vector<std::vector<int>>&) override { return 0; }
		int enqueue(const std::shared_ptr<des::event>&, const std::vector<std::vector<int>>&) override { return 0; }
		int dequeue(const std::shared_ptr<des::event>&, const std::vector<std::vector<int>>&) override { return 0; }
	};

	// Records the smallest value of a field seen in departure messages
	struct min_observer : des::observer
	{
		des::tag field;
		double min = __DBL_MAX__;
		explicit min_observer(des::tag f) : field(f) {}
		void update(const des::message& m) override { min = std::min(min, m.get_value(field)); }
		void reset(bool) override {}
		void reset(int, bool) override {}
		void clear() override {}
		std::string to_string() const override { return des::tag_registry::name(field); }
	};

	bool in_service(const std::shared_ptr<des::event>& e)
	{
		return e -> get_info(NODE_SERVICE_START).first;
	}

	int server_of(const std::shared_ptr<des::event>& e)
	{
		return in_service(e) ? static_cast<int>(e -> get_info(EVENT_SERVER).second) : -1;
	}
}

TEST_CASE("node constructor sizes its counters for every class", "[unit][node]")
{
	auto srv = std::make_shared<des::queue>(1u, std::make_shared<des::fifo>());
	probe_node n(3u, {}, {srv}, {}, {{1, 1, 1}}, "probe");
	CHECK(des_models::arrive(n, 2, 0.0) != nullptr);   // class 2: out of range before the fix
}

TEST_CASE("departure records sojourn, wait and service", "[unit][node]")
{
	det_station sta(det_dists{{fixed(10)}}, 1, 1, 1, INT_MAX, "Q");
	arrive(sta, 0, 0.0);
	arrive(sta, 0, 1.0);
	auto first = sta.departure();
	CHECK_EQ(first -> get_time(), 10.0);
	CHECK_EQ(first -> get_info(NODE_SOJOURN).second, 10.0);
	CHECK_EQ(first -> get_info(NODE_WAIT).second, 0.0);
	auto second = sta.departure();
	CHECK_EQ(second -> get_time(), 20.0);
	CHECK_EQ(second -> get_info(NODE_SOJOURN).second, 19.0);
	CHECK_EQ(second -> get_info(NODE_WAIT).second, 9.0);
	CHECK_EQ(second -> get_info(NODE_SERVICE).second, 10.0);
	CHECK(!second -> get_info(NODE_ARRIVAL).first);   // node-specific fields are cleared
	CHECK(!second -> get_info(EVENT_SERVER).first);
}

TEST_CASE("a class without a mapped queue is refused; without a mapped server it is an error", "[unit][node]")
{
	auto x = expo(1.0);
	exp_station sta(exp_dists{{x, x}}, 1, 1, 1, INT_MAX, "Q");
	sta.set_queue_map({{1, 0}});             // queue 0 only for class 0
	REQUIRE(arrive(sta, 0, 0.0) != nullptr);   // fills the server
	CHECK(arrive(sta, 1, 0.0) == nullptr);     // was accepted into class 0's queue before the fix
	sta.set_server_map({{1, 0}});            // server 0 only for class 0
	CHECK_THROWS_AS(std::runtime_error, arrive(sta, 1, 0.0));
}

TEST_CASE("a freed server takes a waiting job of another class", "[unit][node]")
{
	det_station sta(det_dists{{fixed(1), fixed(1)}}, 1, 1, 2, INT_MAX, "Q");
	sta.set_queue_map({{1, 0}, {0, 1}});     // queue 0: class 0, queue 1: class 1
	arrive(sta, 0, 0.0);
	auto waiting = arrive(sta, 1, 0.0);
	REQUIRE(sta.departure() -> get_cls() == 0);
	CHECK_EQ(sta.service_length(), 1);
	CHECK_EQ(sta.queue_length(), 0);
	CHECK_EQ(server_of(waiting), 0);
}

TEST_CASE("a freed server skips waiting jobs it may not serve", "[unit][node]")
{
	// server 0 slow (10) and only for class 0, server 1 fast (1) and only for class 1
	det_station sta(det_dists{{fixed(10), fixed(10)}, {fixed(1), fixed(1)}}, 2, 1, 1, INT_MAX, "Q");
	sta.set_server_map({{1, 0}, {0, 1}});
	arrive(sta, 0, 0.0);
	arrive(sta, 1, 0.0);
	auto head = arrive(sta, 0, 0.1);         // waits at the head of the queue
	auto next = arrive(sta, 1, 0.2);
	REQUIRE(sta.departure() -> get_cls() == 1);
	CHECK_EQ(server_of(next), 1);
	CHECK_EQ(server_of(head), -1);           // was moved onto the class-1 server before the fix
	CHECK_EQ(sta.queue_length(), 1);
}

TEST_CASE("reset shifts the service start of jobs in service", "[unit][node]")
{
	open_model m({0.9}, 6, [] { return std::vector<std::shared_ptr<des::node>>{mm_station(1.0)}; });
	auto service = std::make_shared<min_observer>(NODE_SERVICE);
	auto wait = std::make_shared<min_observer>(NODE_WAIT);
	m.stations[0] -> attach(SIGNAL_NODE_DEPARTURE, service);
	m.stations[0] -> attach(SIGNAL_NODE_DEPARTURE, wait);
	m.run(0, 5000, 20);
	CHECK(service -> min >= 0.0);            // negative across resets before the fix
	CHECK(wait -> min >= 0.0);
	NOTE("min service " << service -> min << ", min wait " << wait -> min);
}

TEST_CASE("a source draws each class's inter-arrival times from its distribution", "[unit][node][source]")
{
	det_source src({fixed(2), fixed(5)}, "src");
	arrive(src, 0, 0.0);
	arrive(src, 1, 0.0);
	auto next = [&src](int n)
	{
		std::vector<std::pair<int, double>> got;
		for(int i = 0; i < n; i++)
		{
			auto e = src.departure();
			got.push_back({e -> get_cls(), e -> get_time()});
		}
		return got;
	};
	CHECK(next(4) == std::vector<std::pair<int, double>>{{0, 2.0}, {0, 4.0}, {1, 5.0}, {0, 6.0}});
	src.set_rng(fixed(1), 0);
	// The arrival at 8 was drawn before the change
	CHECK(next(2) == std::vector<std::pair<int, double>>{{0, 8.0}, {0, 9.0}});
	CHECK_EQ(src.get_rng(1) -> a(), 5.0);
}

TEST_CASE("a copied node keeps notifying after the original is destroyed", "[unit][node]")
{
	auto original = std::make_unique<exp_station>(exp_dists{{expo(1.0)}}, 1, 1, 1, INT_MAX, "Q");
	auto departures = std::make_shared<des::counter>("departures", 1);
	original -> attach(SIGNAL_NODE_DEPARTURE, departures);
	arrive(*original, 0, 0.0);
	original -> departure();                  // resolves the original's signal lists
	exp_station copy = *original;
	original.reset();
	arrive(copy, 0, 1.0);
	copy.departure();
	CHECK_EQ(departures -> get(0), 2);
}
