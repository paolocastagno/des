#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_random.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

static_assert(std::uniform_random_bit_generator<des::random_engine>);

namespace
{
	// Outputs of the reference SplitMix64 and xoshiro256** (prng.di.unimi.it): three draws
	// after seeding, one after jump(), one after long_jump()
	struct reference
	{
		std::uint64_t seed;
		std::uint64_t draws[3];
		std::uint64_t after_jump;
		std::uint64_t after_long_jump;
	};
	const reference REFERENCE[] = {
		{0, {0x99ec5f36cb75f2b4ULL, 0xbf6e1f784956452aULL, 0x1a5f849d4933e6e0ULL}, 0x06c27b341aca7b26ULL, 0xc012529768639c2dULL},
		{42, {0x15780b2e0c2ec716ULL, 0x6104d9866d113a7eULL, 0xae17533239e499a1ULL}, 0x03a5c66424702131ULL, 0x2e5771502f9237a9ULL},
	};

	/** Routes events until @p n of them departed from a node accepted by @p keep; returns their times. */
	template <typename F>
	std::vector<double> departures(des::network& net, size_t n, F keep)
	{
		std::vector<double> times;
		while(times.size() < n)
		{
			auto e = net.next_event();
			if(keep(e)) times.push_back(e -> get_time());
			net.route(e);
		}
		return times;
	}

	int node_of(const std::shared_ptr<des::event>& e)
	{
		return static_cast<int>(e -> get_info(EVENT_NODE).second);
	}
}

TEST_CASE("random_engine reproduces the reference xoshiro256**", "[unit][random]")
{
	for(const reference& r : REFERENCE)
	{
		des::random_engine g(r.seed);
		for(std::uint64_t v : r.draws) CHECK_EQ(g(), v);
		g.jump();
		CHECK_EQ(g(), r.after_jump);
		g.long_jump();
		CHECK_EQ(g(), r.after_long_jump);
	}
}

TEST_CASE("stream k of a block starts k jumps after the block, whatever the use of the others", "[unit][random]")
{
	const des::random_engine start(7);
	des::stream_block block(start);
	des::random_engine& first = block.at(0);
	for(int i = 0; i < 100; i++) block.at(3)();   // stream 3 is created and used before streams 1 and 2
	des::random_engine expected = start;
	for(size_t k = 0; k < 6; k++)
	{
		if(k != 3) CHECK(block.at(k) == expected);
		expected.jump();
	}
	CHECK(&first == &block.at(0));                // references survive the growth of the block
}

TEST_CASE("each node of a network draws from its own streams", "[unit][random]")
{
	auto a = mm_station(1.0), b = mm_station(1.0);
	des::network net({a, b}, chain({1, 0}), 1);
	a -> arrival(make_event(0, 0.0, 0));
	b -> arrival(make_event(0, 0.0, 1));
	CHECK(a -> next_event_time() != b -> next_event_time());
}

TEST_CASE("nodes added after the others leave their streams unchanged", "[unit][random]")
{
	auto run = [](bool extra)
	{
		auto src = std::make_shared<des::source>(std::vector<double>{0.8}, "src");
		std::vector<std::shared_ptr<des::node>> nodes{src, mm_station(1.0), std::make_shared<des::sink>("snk")};
		std::vector<int> next{1, 2, -1};
		// A second source and station, drawing their own random numbers, feeding the same sink
		auto src2 = std::make_shared<des::source>(std::vector<double>{0.5}, "src2");
		if(extra)
		{
			nodes.push_back(src2);
			nodes.push_back(mm_station(2.0));
			next.push_back(4);
			next.push_back(2);
		}
		des::network net(nodes, chain(next), 5);
		src -> arrival(make_event(0, 0.0, 0));
		if(extra) src2 -> arrival(make_event(0, 0.0, 3));
		return departures(net, 2000, [](const auto& e) { return node_of(e) <= 1; });
	};
	CHECK(run(false) == run(true));
}

TEST_CASE("the arrivals of a class do not depend on the rates of the other classes", "[unit][random]")
{
	auto arrivals = [](double rate1)
	{
		auto src = std::make_shared<des::source>(std::vector<double>{1.0, rate1}, "src");
		auto snk = std::make_shared<des::sink>("snk", 2);
		des::network net({src, snk}, chain({1, -1}, 2), 3);
		for(int c = 0; c < 2; c++) src -> arrival(make_event(c, 0.0));
		return departures(net, 1000, [](const auto& e) { return node_of(e) == 0 && e -> get_cls() == 0; });
	};
	CHECK(arrivals(1.0) == arrivals(4.0));
}
