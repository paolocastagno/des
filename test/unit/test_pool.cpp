#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <libdes_const.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

namespace
{
	double mm1_mean_sojourn(unsigned long long seed)
	{
		open_model m({0.8}, seed, [] { return std::vector<std::shared_ptr<des::node>>{mm_station(1.0)}; });
		m.run(0, 100000, 2);
		return m.sojourn[0] -> get_scalar(0);
	}
}

TEST_CASE("events returned by next_event are not recycled while held", "[unit][pool]")
{
	auto src = poisson_source({1.0}, "src");
	auto snk = std::make_shared<des::sink>("snk");
	des::network net({src, snk}, chain({1, -1}), 21);
	src -> arrival(make_event(0, 0.0));
	std::vector<std::pair<std::shared_ptr<des::event>, double>> kept;
	for(int i = 0; i < 2000; i++)
	{
		auto e = net.next_event();
		net.route(e);                         // the sink absorbs e and returns it to the pool
		if(i % 50 == 0)
			kept.push_back({e, e -> get_time()});
	}
	REQUIRE(!kept.empty());
	for(auto& [e, t] : kept)
	{
		CHECK_EQ(e -> get_time(), t);
		CHECK_EQ(e -> get_info(EVENT_NODE).second, 1.0);
	}
}

TEST_CASE("replications on parallel threads match sequential runs", "[unit][threads][pool]")
{
	const unsigned long long seeds[] = {31, 32, 33, 34};
	double sequential[4], parallel[4];
	for(int i = 0; i < 4; i++) sequential[i] = mm1_mean_sojourn(seeds[i]);
	std::vector<std::thread> threads;
	for(int i = 0; i < 4; i++)
		threads.emplace_back([&, i]{ parallel[i] = mm1_mean_sojourn(seeds[i]); });
	for(std::thread& t : threads) t.join();
	for(int i = 0; i < 4; i++) CHECK_EQ(parallel[i], sequential[i]);
}
