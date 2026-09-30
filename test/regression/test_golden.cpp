// Regression tier: fixed-seed models whose results must not change unintentionally.
//
// <random> distributions are implementation-defined and compilers may fuse floating-point
// operations differently, so the expected values are kept per platform in
// DES_GOLDEN_DIR/<platform>/<model>.txt. A platform without golden values reports the
// tests as skipped; `make golden` writes them (review the diff before committing).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <libdes_const.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

#ifndef DES_GOLDEN_DIR
#define DES_GOLDEN_DIR "test/regression/golden"
#endif

using namespace des_models;

namespace
{
	using values = std::vector<std::pair<std::string, double>>;

	std::string platform()
	{
#if defined(_LIBCPP_VERSION)
		std::string lib = "libcxx";
#elif defined(__GLIBCXX__)
		std::string lib = "libstdcxx";
#else
		std::string lib = "stdlib";
#endif
#if defined(__clang__)
		std::string cc = "clang";
#elif defined(__GNUC__)
		std::string cc = "gcc";
#else
		std::string cc = "cc";
#endif
#if defined(__aarch64__) || defined(__arm64__)
		std::string arch = "arm64";
#elif defined(__x86_64__)
		std::string arch = "x86_64";
#else
		std::string arch = "arch";
#endif
		return lib + "-" + cc + "-" + arch;
	}

	/** Compares @p got with the golden values of @p model, or writes them with --update-golden. */
	void golden(const std::string& model, const values& got)
	{
		std::filesystem::path dir = std::filesystem::path(DES_GOLDEN_DIR) / platform();
		std::filesystem::path file = dir / (model + ".txt");
		if(des_test::update_golden())
		{
			std::filesystem::create_directories(dir);
			std::ofstream out(file);
			for(const auto& [name, v] : got)
			{
				char buf[64];
				std::snprintf(buf, sizeof(buf), "%.17g", v);
				out << name << " " << buf << "\n";
			}
			NOTE("wrote " << file.string());
			return;
		}
		std::ifstream in(file);
		if(!in)
			SKIP("no golden values for " << platform() << " (" << file.string() << "): run `make golden`");
		std::map<std::string, double> expected;
		std::string name;
		double v;
		while(in >> name >> v) expected[name] = v;
		CHECK_EQ(expected.size(), got.size());
		for(const auto& [n, value] : got)
		{
			auto it = expected.find(n);
			if(it == expected.end())
			{
				des_test::fail(__FILE__, __LINE__, "no golden value for " + n);
				continue;
			}
			double tol = 1e-9 * std::max(1.0, std::fabs(it -> second));
			if(!(std::fabs(value - it -> second) <= tol))
				des_test::fail(__FILE__, __LINE__, model + ": " + n + " = " + des_test::show(value) + ", golden " + des_test::show(it -> second));
		}
	}

	std::vector<std::shared_ptr<des::node>> one(std::shared_ptr<des::node> n)
	{
		return {n};
	}

	/** Per-run sojourn means and throughput of an open model. */
	values open_values(open_model& m, int classes)
	{
		values out;
		for(size_t s = 0; s < m.sojourn.size(); s++)
			for(int c = 0; c < classes; c++)
			{
				auto ci = m.sojourn[s] -> confidence_interval(0.05, c);
				out.push_back({"sojourn_" + std::to_string(s) + "_" + std::to_string(c) + "_mean", m.sojourn[s] -> get_scalar(c)});
				out.push_back({"sojourn_" + std::to_string(s) + "_" + std::to_string(c) + "_ci_low", ci.first});
			}
		int last = static_cast<int>(m.stations.size());
		for(int c = 0; c < classes; c++)
			out.push_back({"throughput_" + std::to_string(c), m.net -> get_flow(last, last + 1, c)});
		out.push_back({"end_time", m.last_time});
		return out;
	}
}

TEST_CASE("golden: M/M/1", "[regression]")
{
	open_model m({0.8}, 101, [] { return one(mm_station(1.0)); });
	m.run(10000, 50000, 3);
	golden("mm1", open_values(m, 1));
}

TEST_CASE("golden: M/M/2", "[regression]")
{
	open_model m({1.6}, 102, [] { return one(mm_station(1.0, 2)); });
	m.run(10000, 50000, 3);
	golden("mm2", open_values(m, 1));
}

TEST_CASE("golden: tandem of two stations", "[regression]")
{
	open_model m({0.8}, 103, [] { return std::vector<std::shared_ptr<des::node>>{mm_station(1.0), mm_station(1.25)}; });
	m.run(10000, 50000, 3);
	golden("tandem", open_values(m, 1));
}

TEST_CASE("golden: two-class weighted PS", "[regression]")
{
	open_model m({0.3, 0.4}, 104, []
	{
		return one(std::make_shared<exp_station>(exp_dists{{expo(1.0), expo(0.8)}}, 1, UNLIMITED, std::make_shared<des::ps>(std::vector<double>{2.0, 1.0}), "gps"));
	});
	m.run(10000, 50000, 3);
	golden("gps", open_values(m, 2));
}

TEST_CASE("golden: infinite server", "[regression]")
{
	open_model m({20.0}, 105, [] { return one(std::make_shared<exp_station>(exp_dists{{expo(1.0)}}, 1, UNLIMITED, std::make_shared<des::is>(), "is")); });
	m.run(10000, 50000, 3);
	golden("is", open_values(m, 1));
}

TEST_CASE("golden: loss station", "[regression]")
{
	open_model m({1.5}, 106, [] { return one(mm_station(1.0, 2, 0)); });
	m.step(60000);
	values v{{"routed", static_cast<double>(m.net -> get_count(0, 1, 0))},
			 {"blocked", static_cast<double>(m.net -> get_blocked(0, 1, 0))},
			 {"lost", static_cast<double>(m.net -> get_lost(0, 0))},
			 {"end_time", m.last_time}};
	golden("loss", v);
}

TEST_CASE("golden: closed network", "[regression]")
{
	auto a = mm_station(1.0), b = mm_station(2.0);
	des::network net({a, b}, chain({1, 0}), 107);
	for(int k = 0; k < 3; k++) a -> arrival(make_event(0, 0.0, 0));
	double t = 0;
	for(int r = 0; r < 3; r++)
	{
		for(int i = 0; i < 50000; i++) { auto e = net.next_event(); t = e -> get_time(); net.route(e); }
		net.reset(t, {}, true);
	}
	golden("closed", values{{"throughput_01", net.get_flow(0, 1, 0)}, {"throughput_10", net.get_flow(1, 0, 0)}});
}

TEST_CASE("the same seed reproduces a run exactly", "[regression]")
{
	auto run = [](unsigned long long seed)
	{
		open_model m({0.8}, seed, [] { return one(mm_station(1.0)); });
		m.run(1000, 20000, 2);
		return open_values(m, 1);
	};
	values a = run(7), b = run(7), c = run(8);
	CHECK(a == b);
	CHECK(a != c);
}
