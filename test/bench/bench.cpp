// Benchmarks: time reference models and print a Markdown table (events per second).
// Not a pass/fail test: compare the numbers across commits on the same machine.
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "des_models.hpp"

using namespace des_models;

namespace
{
	void bench(const char* name, long events, const std::function<void(long)>& run)
	{
		auto t0 = std::chrono::steady_clock::now();
		run(events);
		double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
		std::printf("| %-34s | %10ld | %8.3f | %8.2f |\n", name, events, s, events / s / 1e6);
		std::fflush(stdout);
	}

	std::function<void(long)> open(std::vector<double> lambda, std::function<std::vector<std::shared_ptr<des::node>>()> make)
	{
		return [lambda, make](long events)
		{
			open_model m(lambda, 1, make);
			m.step(static_cast<int>(events));
		};
	}

	std::vector<std::shared_ptr<des::node>> one(std::shared_ptr<des::node> n)
	{
		return {n};
	}
}

int main()
{
	std::printf("| %-34s | %10s | %8s | %8s |\n", "model", "events", "seconds", "Mev/s");
	std::printf("|%s|%s|%s|%s|\n", std::string(36, '-').c_str(), std::string(12, '-').c_str(), std::string(10, '-').c_str(), std::string(10, '-').c_str());
	bench("M/M/1 (rho 0.8)", 3000000, open({0.8}, [] { return one(mm_station(1.0)); }));
	bench("M/M/2 (rho 0.8)", 3000000, open({1.6}, [] { return one(mm_station(1.0, 2)); }));
	bench("tandem of 5 stations", 3000000, open({0.8}, []
	{
		std::vector<std::shared_ptr<des::node>> s;
		for(int i = 0; i < 5; i++) s.push_back(mm_station(1.0));
		return s;
	}));
	bench("M/M/1-PS (rho 0.98, ~50 jobs)", 2000000, open({0.98}, []
	{
		return one(std::make_shared<exp_station>(exp_dists{{expo(1.0)}}, 1, UNLIMITED, std::make_shared<des::ps>(), "ps"));
	}));
	bench("M/M/inf (~200 jobs)", 2000000, open({200.0}, []
	{
		return one(std::make_shared<exp_station>(exp_dists{{expo(1.0)}}, 1, UNLIMITED, std::make_shared<des::is>(), "is"));
	}));
	bench("closed network, 60 stations", 3000000, [](long events)
	{
		const int n = 60;
		std::vector<std::shared_ptr<des::node>> nodes;
		for(int i = 0; i < n; i++) nodes.push_back(mm_station(1.0 + i % 5));
		std::vector<int> next(n);
		for(int i = 0; i < n; i++) next[i] = (i + 1) % n;
		des::network net(nodes, chain(next), 1);
		for(int k = 0; k < 120; k++) nodes[k % n] -> arrival(make_event(0, 0.0, k % n));
		for(long i = 0; i < events; i++) net.route(net.next_event());
	});
	return 0;
}
