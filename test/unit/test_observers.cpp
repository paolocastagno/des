#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_counter.hpp>
#include <libdes_histogram.hpp>
#include <libdes_message.hpp>
#include <libdes_observable.hpp>
#include <libdes_ratio.hpp>
#include <libdes_sample.hpp>
#include <libdes_scalar.hpp>
#include <libdes_sink.hpp>
#include <libdes_util.hpp>

#include "des_test.hpp"

namespace
{
	des::message with(des::tag field, double value, int cls)
	{
		des::message m;
		m.add(field, value);
		m.add(EVENT_CLS, cls);
		return m;
	}
}

TEST_CASE("counter counts per class and keeps completed runs", "[unit][observer]")
{
	des::counter c("c", 2);
	c.update(with(NODE_SOJOURN, 1.0, 1));
	c.update(0);
	c.update(1);
	CHECK_EQ(c.get(0), 1);
	CHECK_EQ(c.get(1), 2);
	c.reset(true);
	CHECK_EQ(c.get(1), 0);
	c.update(1);
	c.reset(true);
	auto ci = c.confidence_interval(0.05, 1);   // runs {2, 1}
	CHECK(ci.first < 1.5 && ci.second > 1.5);
}

TEST_CASE("scalar: running mean and variance, cross-run mean", "[unit][observer]")
{
	des::scalar s(NODE_SOJOURN, 1);
	CHECK_EQ(s.get_observer(), std::string("node_sojourn"));   // named after its field
	for(double v : {2.0, 4.0, 6.0}) s.update(with(NODE_SOJOURN, v, 0));
	CHECK_NEAR(s.mean(0), 4.0, 1e-12);
	CHECK_NEAR(s.run_stddev(0), 2.0, 1e-12);
	s.reset(true);
	s.update(with(NODE_SOJOURN, 8.0, 0));
	s.reset(true);
	CHECK_EQ(s.completed_runs(0), 2u);
	CHECK_NEAR(s.get_scalar(0), 6.0, 1e-12);
	s.update(with(NODE_SOJOURN, 100.0, 0));
	s.reset(false);                           // discarded, e.g. a warm-up
	CHECK_EQ(s.completed_runs(0), 2u);
}

TEST_CASE("confidence intervals use the Student-t quantile", "[unit][observer]")
{
	// {1, 2, 3}: mean 2, s = 1, t(0.975, 2) = 4.302653 -> 2 +- 2.484138
	auto ci = conf_int(std::vector<double>{1.0, 2.0, 3.0}, 0.05);
	CHECK_NEAR(ci.first, 2.0 - 2.484138, 1e-5);
	CHECK_NEAR(ci.second, 2.0 + 2.484138, 1e-5);
	CHECK_NEAR(student_t_quantile(0.975, 10), 2.228139, 1e-5);
	CHECK_NEAR(student_t_quantile(0.9995, 9), 4.780913, 1e-5);
}

TEST_CASE("without enough runs the interval is unbounded", "[unit][observer]")
{
	des::counter c("c", 1);
	des::scalar s("s", 1);
	des::sample m("m", 1);
	des::ratio r("r", 1);
	r.update(1.0, 2.0, 0);   // one pair: no interval within the run either
	for(auto ci : {c.confidence_interval(0.05, 0), s.confidence_interval(0.05, 0), m.confidence_interval(0.05, 0),
				   r.confidence_interval(0.05, 0), r.run_confidence_interval(0.05, 0)})
	{
		CHECK_EQ(ci.first, -__DBL_MAX__);
		CHECK_EQ(ci.second, __DBL_MAX__);
	}
}

TEST_CASE("ratio: estimate and interval within a run", "[unit][observer]")
{
	des::ratio r("throughput", 2);
	for(auto [x, y] : std::vector<std::pair<double, double>>{{1, 2}, {3, 4}, {2, 2}, {6, 8}}) r.update(x, y, 0);
	CHECK_EQ(r.n_updates(0), 4);
	CHECK_EQ(r.n_updates(1), 0);
	CHECK_NEAR(r.get(0), 0.75, 1e-12);                 // 12 / 16
	CHECK_EQ(r.get(1), 0.0);                           // no pairs yet
	// x - 0.75 y = {-0.5, 0, 0.5, 0}: s^2 = 0.5 / 3; half-width t(0.975, 3) * s / (4 * 2)
	auto ci = r.run_confidence_interval(0.05, 0);
	CHECK_NEAR(ci.first, 0.587596467, 1e-6);
	CHECK_NEAR(ci.second, 0.912403533, 1e-6);
}

TEST_CASE("ratio: the interval accounts for the covariance of x and y", "[unit][observer]")
{
	// x and y vary, but x = 2y in every pair: the ratio is exactly 2
	des::ratio r("r", 1);
	for(double y : {1.0, 3.0, 5.0, 2.0}) r.update(2 * y, y, 0);
	auto ci = r.run_confidence_interval(0.05, 0);
	CHECK_NEAR(ci.first, 2.0, 1e-12);
	CHECK_NEAR(ci.second, 2.0, 1e-12);
}

TEST_CASE("ratio: online moments match a two-pass computation", "[unit][observer]")
{
	// Large, nearly equal values, where sums of squares would lose the variance to cancellation
	std::vector<std::pair<double, double>> pairs;
	for(int i = 0; i < 1000; i++) pairs.push_back({1e8 + (i * 7919) % 13, 2e8 + (i * 104729) % 17});
	des::ratio r("r", 1);
	double sx = 0, sy = 0;
	for(auto [x, y] : pairs) { r.update(x, y, 0); sx += x; sy += y; }
	const double k = static_cast<double>(pairs.size()), q = sx / sy;
	double s2 = 0;
	for(auto [x, y] : pairs) s2 += (x - q * y) * (x - q * y);
	s2 /= k - 1;
	const double half = student_t_quantile(0.975, k - 1) * std::sqrt(s2) / (sy / k * std::sqrt(k));
	auto ci = r.run_confidence_interval(0.05, 0);
	CHECK_NEAR(r.get(0), q, 1e-15);
	CHECK_NEAR(ci.second - ci.first, 2 * half, 1e-6 * half);
}

TEST_CASE("ratio: completed runs and the interval across runs", "[unit][observer]")
{
	des::ratio r("r", 2);
	for(auto [x, y] : std::vector<std::pair<double, double>>{{1, 2}, {3, 4}, {2, 2}, {6, 8}}) r.update(x, y, 0);
	r.reset(true);                                     // run estimate 0.75
	CHECK_EQ(r.n_updates(0), 0);
	r.update(1, 2, 0);
	r.update(2, 4, 0);
	r.reset(true);                                     // run estimate 0.5
	r.update(9, 1, 0);
	r.reset(false);                                    // discarded, e.g. a warm-up
	CHECK_EQ(r.completed_runs(0), 2u);
	CHECK_EQ(r.completed_runs(1), 0u);                 // a class without pairs stores nothing
	CHECK_NEAR(r.get_ratio(0), 0.625, 1e-12);
	// {0.75, 0.5}: mean 0.625, s = 0.176777, t(0.975, 1) = 12.706205
	auto ci = r.confidence_interval(0.05, 0);
	CHECK_NEAR(ci.first, -0.963275592, 1e-6);
	CHECK_NEAR(ci.second, 2.213275592, 1e-6);
	CHECK_EQ(r.confidence_interval(0.05).size(), 2u);
	r.clear();
	CHECK_EQ(r.completed_runs(0), 0u);
}

TEST_CASE("sample keeps every observation", "[unit][observer]")
{
	des::sample m(NODE_WAIT, 2);
	m.update(with(NODE_WAIT, 1.0, 0));
	m.update(3.0, 0);
	m.update(5.0, 1);
	CHECK_EQ(m.observations(0), 2u);
	CHECK_NEAR(m.mean(0), 2.0, 1e-12);
	std::string s = m.to_string();
	CHECK(s.find("(0)") != std::string::npos && s.find("(1)") != std::string::npos);
}

TEST_CASE("histogram bins by index and rejects values it cannot bin", "[unit][observer]")
{
	des::histogram h(NODE_SOJOURN, 2);
	h.set_binsize(1.0);
	for(double x : {0.5, 2.2, 2.9, 0.1, 5.0}) h.update(x, 0);
	h.update(with(NODE_SOJOURN, 0.3, 1));
	auto bins = h.get().at(0);
	REQUIRE(bins.size() == 6u);
	CHECK_EQ(bins[0].count, 2u);
	CHECK_EQ(bins[1].count, 0u);
	CHECK_EQ(bins[2].count, 2u);
	CHECK_EQ(bins[5].count, 1u);
	for(size_t k = 0; k < bins.size(); k++) CHECK_EQ(bins[k].value, static_cast<double>(k));
	CHECK_EQ(h.observations(1), 1u);
	CHECK_THROWS_AS(std::invalid_argument, h.update(-1.0, 0));
	CHECK_THROWS_AS(std::invalid_argument, h.update(std::nan(""), 0));
	CHECK_THROWS_AS(std::invalid_argument, h.update(1e20, 0));
	std::string s = h.to_string();
	CHECK(s.find("(0)") != std::string::npos && s.find("(1)") != std::string::npos);
}

TEST_CASE("observers built without a field reject messages", "[unit][observer]")
{
	des::message m = with(NODE_SOJOURN, 1.0, 0);
	CHECK_THROWS_AS(std::logic_error, des::scalar("flow", 1).update(m));
	CHECK_THROWS_AS(std::logic_error, des::sample("x", 1).update(m));
	CHECK_THROWS_AS(std::logic_error, des::histogram("x", 1).update(m));
	CHECK_THROWS_AS(std::logic_error, des::ratio("x", 1).update(m));
	des::scalar fed("flow", 1);
	fed.update(1.5, 0);
	CHECK_EQ(fed.mean(0), 1.5);
}

TEST_CASE("attaching to an unknown signal names the signal", "[unit][observer]")
{
	des::sink s("S");
	bool named = false;
	try { s.attach("unit_test_no_such_signal", std::make_shared<des::counter>("c", 1)); }
	catch(const std::invalid_argument& e) { named = std::string(e.what()).find("unit_test_no_such_signal") != std::string::npos; }
	CHECK(named);
}
