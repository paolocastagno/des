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
	for(auto ci : {c.confidence_interval(0.05, 0), s.confidence_interval(0.05, 0), m.confidence_interval(0.05, 0)})
	{
		CHECK_EQ(ci.first, -__DBL_MAX__);
		CHECK_EQ(ci.second, __DBL_MAX__);
	}
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
