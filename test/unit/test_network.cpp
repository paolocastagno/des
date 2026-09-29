#include <cmath>
#include <memory>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_network.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

namespace
{
	struct exposed_network : des::network
	{
		using des::network::network;
		using des::network::insert;
	};

	int reroute_calls = 0;
	int reroute_target = 0;
	std::pair<bool, int> to_target(const std::shared_ptr<des::event>&, int, const routing_t&, std::shared_ptr<std::mt19937_64>&)
	{
		++reroute_calls;
		return {true, reroute_target};
	}

	int source_to_1(const std::shared_ptr<des::event>& e, const routing_t&, std::shared_ptr<std::mt19937_64>&)
	{
		return e -> get_info(EVENT_NODE).second == 0 ? 1 : -1;
	}

	// Loss station: one server, one place, no waiting room
	std::shared_ptr<des::node> loss_station(std::shared_ptr<std::mt19937_64> g, double mu)
	{
		return mm_station(g, mu, 1, 0);
	}
}

TEST_CASE("insert: unknown node refused, known node scheduled", "[unit][network]")
{
	auto gen = engine(11);
	auto a = std::make_shared<des::source>(std::vector<double>{1.0}, "A", gen);
	auto b = std::make_shared<des::source>(std::vector<double>{1.0}, "B", gen);
	exposed_network net({a, b}, chain({-1, -1}), gen);
	CHECK(!net.insert(make_event(0, 0.0, 1), "nope"));
	REQUIRE(net.insert(make_event(0, 0.0, 1), "B"));
	REQUIRE(net.next_event() != nullptr);     // the heap now holds B and never empties again
	REQUIRE(net.insert(make_event(0, 0.0, 0), "A"));
	bool saw_a = false;
	for(int i = 0; i < 50 && !saw_a; i++)
		saw_a = net.next_event() -> get_info(EVENT_NODE).second == 0;
	CHECK(saw_a);
}

TEST_CASE("route works with every network observer detached", "[unit][network]")
{
	auto gen = engine(12);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0}, "src", gen);
	auto snk = std::make_shared<des::sink>("snk");
	des::network net({src, snk}, chain({1, -1}), gen);
	net.detach();
	src -> arrival(make_event(0, 0.0));
	CHECK_NOTHROW(for(int i = 0; i < 1000; i++) net.route(net.next_event()));
}

TEST_CASE("node 0 can be a routing destination", "[unit][network]")
{
	auto gen = engine(13);
	auto a = mm_station(gen, 1.0), b = mm_station(gen, 1.0);
	des::network net({a, b}, chain({1, 0}), gen);
	for(int k = 0; k < 3; k++) a -> arrival(make_event(0, 0.0, 0));
	for(int i = 0; i < 10000; i++)
	{
		auto e = net.next_event();
		REQUIRE(e != nullptr);                // the population leaked on every 1 -> 0 hop before the fix
		net.route(e);
	}
	CHECK(net.get_count(1, 0, 0) > 0);
}

TEST_CASE("blocked events are lost and counted", "[unit][network]")
{
	auto gen = engine(14);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0}, "src", gen);
	auto sta = loss_station(gen, 1.0);
	auto snk = std::make_shared<des::sink>("snk");
	des::network net({src, sta, snk}, chain({1, 2, -1}), gen);
	src -> arrival(make_event(0, 0.0));
	int from_source = 0;
	for(int i = 0; i < 40000; i++)
	{
		auto e = net.next_event();
		if(e -> get_info(EVENT_NODE).second == 0) ++from_source;
		net.route(e);
	}
	int routed = net.get_count(0, 1, 0), blocked = net.get_blocked(0, 1, 0), lost = net.get_lost(0, 0);
	CHECK(blocked > 0);
	CHECK_EQ(blocked, lost);
	CHECK_EQ(routed + lost, from_source);
}

TEST_CASE("a reroute is counted on the edge actually used", "[unit][network]")
{
	auto gen = engine(15);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0}, "src", gen);
	auto full = loss_station(gen, 1.0);
	auto spare = mm_station(gen, 1.0);
	auto snk = std::make_shared<des::sink>("snk");
	reroute_target = 2;
	des::network net({src, full, spare, snk}, chain({1, 3, 3, -1}), to_target, gen);
	src -> arrival(make_event(0, 0.0));
	for(int i = 0; i < 40000; i++) net.route(net.next_event());
	CHECK(net.get_blocked(0, 1, 0) > 0);
	CHECK_EQ(net.get_count(0, 2, 0), net.get_blocked(0, 1, 0));
	CHECK_EQ(net.get_lost(0, 0), 0);
}

TEST_CASE("reroutes to a full node are bounded", "[unit][network]")
{
	auto gen = engine(16);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0}, "src", gen);
	auto busy = loss_station(gen, 1e-9);     // effectively never completes
	auto snk = std::make_shared<des::sink>("snk");
	reroute_target = 1;
	reroute_calls = 0;
	des::network net({src, busy, snk}, chain({1, 2, -1}), to_target, gen);
	net.set_max_reroute_attempts(3);
	src -> arrival(make_event(0, 0.0));
	for(int i = 0; i < 101; i++) net.route(net.next_event());   // 1 accepted, then 100 blocked
	CHECK_EQ(net.get_lost(0, 0), 100);
	CHECK_EQ(reroute_calls, 300);
	CHECK_EQ(net.get_blocked(0, 1, 0), 400);                    // first try + 3 reroutes each
}

TEST_CASE("flow is N/T per run and N/now within a run", "[unit][network]")
{
	open_model m({0.8}, 17, [](std::shared_ptr<std::mt19937_64> g) { return std::vector<std::shared_ptr<des::node>>{mm_station(g, 1.0)}; });
	std::vector<double> per_run;
	for(int r = 0; r < 4; r++)
	{
		m.step(20000);
		CHECK_NEAR(m.net -> flow(1, 2, 0), m.net -> get_count(1, 2, 0) / m.last_time, 1e-12);
		per_run.push_back(m.net -> get_count(1, 2, 0) / m.last_time);
		m.net -> reset(m.last_time, {}, true);
	}
	double mean = 0;
	for(double v : per_run) mean += v / per_run.size();
	CHECK_NEAR(m.net -> get_flow(1, 2, 0), mean, 1e-12);
	CHECK_EQ(m.net -> flow(1, 2, 0), 0.0);
}

TEST_CASE("per-edge class counts: an edge tracks the classes it lists", "[unit][network]")
{
	auto gen = engine(18);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0, 1.0}, "src", gen);
	auto snk = std::make_shared<des::sink>("snk", 2);
	// 0 -> 1 carries 2 classes, the other edges list only 1
	des::network net({src, snk}, {{{0}, {1, 1}}, {{0}, {0}}}, source_to_1, gen);
	for(int c = 0; c < 2; c++) src -> arrival(make_event(c, 0.0));
	double t = 0;
	for(int i = 0; i < 2000; i++) { auto e = net.next_event(); t = e -> get_time(); net.route(e); }
	CHECK(net.get_count(0, 1, 1) > 0);
	net.reset(t, {}, true);
	CHECK(net.get_flow(0, 1, 1) > 0);
	CHECK_EQ(net.get_count(0, 1, 1), 0);
	CHECK_EQ(net.get_lost(0, 1), 0);
}

TEST_CASE("next_event returns departures in (time, node) order", "[unit][network]")
{
	const int n = 60;
	auto gen = engine(19);
	std::vector<std::shared_ptr<des::node>> nodes;
	for(int i = 0; i < n; i++)
	{
		unsigned int servers = 1 + i % 3;
		nodes.push_back(std::make_shared<exp_station>(exp_dists(servers, {expo(0.5 + (i % 7) * 0.3)}), servers, 1, 1, INT_MAX, "S", gen));
	}
	routing_t rtg(n, std::vector<std::vector<double>>(n, {0.0}));
	std::mt19937_64 r(7);
	for(int i = 0; i < n; i++)
	{
		double tot = 0;
		std::vector<double> w(n);
		for(int j = 0; j < n; j++) { w[j] = std::uniform_real_distribution<double>(0, 1)(r); tot += w[j]; }
		for(int j = 0; j < n; j++) rtg[i][j][0] = w[j] / tot;
	}
	des::network net(nodes, rtg, gen);
	for(int k = 0; k < 200; k++) nodes[k % n] -> arrival(make_event(0, 0.0, k % n));   // many ties at t = 0
	int wrong = 0;
	for(int step = 0; step < 100000; step++)
	{
		double tmin = __DBL_MAX__;
		int imin = -1;
		for(int i = 0; i < n; i++)
		{
			double t = nodes[i] -> next_event_time();
			if(t < tmin) { tmin = t; imin = i; }
		}
		auto e = net.next_event();
		if(e -> get_time() != tmin || e -> get_info(EVENT_NODE).second != imin) ++wrong;
		net.route(e);
	}
	CHECK_EQ(wrong, 0);
}

TEST_CASE("a copied network keeps counting after the original is destroyed", "[unit][network]")
{
	auto gen = engine(20);
	auto src = std::make_shared<des::source>(std::vector<double>{1.0}, "src", gen);
	auto sta = loss_station(gen, 2.0);
	auto snk = std::make_shared<des::sink>("snk");
	auto original = std::make_unique<des::network>(std::vector<std::shared_ptr<des::node>>{src, sta, snk}, chain({1, 2, -1}), gen);
	src -> arrival(make_event(0, 0.0));
	for(int i = 0; i < 1000; i++) original -> route(original -> next_event());
	des::network copy = *original;
	original.reset();
	int routed = copy.get_count(0, 1, 0), lost = copy.get_lost(0, 0);
	for(int i = 0; i < 1000; i++) copy.route(copy.next_event());
	CHECK(copy.get_count(0, 1, 0) > routed);
	CHECK(copy.get_lost(0, 0) > lost);
}
