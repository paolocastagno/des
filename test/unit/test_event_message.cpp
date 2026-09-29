#include <stdexcept>
#include <string>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_message.hpp>

#include "des_test.hpp"

TEST_CASE("event fields are addressed by tag", "[unit][event]")
{
	des::tag prio = des::tag_registry::define("unit_test_prio");
	des::tag weight = des::tag_registry::define("unit_test_weight");
	des::event e(1, {{prio, 3.0}, {weight, 0.5}});
	CHECK_EQ(e.get_cls(), 1);
	CHECK_EQ(e.get_info(prio).second, 3.0);
	CHECK_EQ(e.get_info(weight).second, 0.5);
	CHECK(!e.set_info(prio, 9.0));               // set_info never overwrites
	CHECK_EQ(e.get_info(prio).second, 3.0);
	e.emplace_info(prio, 9.0);                  // emplace_info does
	CHECK_EQ(e.get_info(prio).second, 9.0);
	e.remove_info(prio);
	CHECK(!e.get_info(prio).first);
	CHECK(e.is_initialized());
	CHECK(e.to_string().find("unit_test_weight:") != std::string::npos);
}

TEST_CASE("time and constraint have dedicated setters only", "[unit][event]")
{
	des::event e;
	CHECK_THROWS_AS(std::runtime_error, e.get_time());         // not set yet
	CHECK_THROWS_AS(std::invalid_argument, e.emplace_info(EVENT_TIME, 1.0));
	CHECK_THROWS_AS(std::invalid_argument, e.set_info(EVENT_CONSTRAINT, 1.0));
	CHECK_THROWS_AS(std::invalid_argument, e.remove_info(EVENT_TIME));
	CHECK_THROWS_AS(std::invalid_argument, des::event(0, {{EVENT_TIME, 1.0}}));
	e.set_time(2.5);
	e.set_constraint(4.0);
	CHECK_EQ(e.get_time(), 2.5);
	CHECK((e.get_constraint() == std::pair<bool, double>(true, 4.0)));
}

TEST_CASE("clone copies every field but the identifier", "[unit][event]")
{
	des::event a(2);
	a.set_time(3.0);
	a.emplace_info(EVENT_NODE, 4);
	des::event b;
	b.clone(a);
	CHECK(b.get_id() != a.get_id());
	CHECK_EQ(b.get_info(EVENT_ID).second, static_cast<double>(b.get_id()));
	CHECK_EQ(b.get_cls(), 2);
	CHECK_EQ(b.get_time(), 3.0);
	CHECK_EQ(b.get_info(EVENT_NODE).second, 4.0);
	b.clear();
	CHECK_EQ(b.get_cls(), 0);
	CHECK(!b.get_info(EVENT_NODE).first);
	CHECK_EQ(b.get_info(EVENT_ID).second, static_cast<double>(b.get_id()));
}

TEST_CASE("shift_times moves the time and the listed fields back, clamping at 0", "[unit][event]")
{
	des::event e;
	e.set_time(10.0);
	e.emplace_info(NODE_ARRIVAL, 3.0);
	e.emplace_info(NODE_SERVICE_START, 8.0);
	e.emplace_info(NODE_SOJOURN, 7.0);          // not listed: untouched
	e.shift_times(5.0, {NODE_ARRIVAL, NODE_SERVICE_START});
	CHECK_EQ(e.get_time(), 5.0);
	CHECK_EQ(e.get_info(NODE_ARRIVAL).second, 0.0);
	CHECK_EQ(e.get_info(NODE_SERVICE_START).second, 3.0);
	CHECK_EQ(e.get_info(NODE_SOJOURN).second, 7.0);
}

TEST_CASE("events compare by time", "[unit][event]")
{
	des::event a, b;
	a.set_time(1.0);
	b.set_time(2.0);
	CHECK(a < b);
	CHECK(b > a);
	CHECK(a <= a);
	CHECK(a != b);
}

TEST_CASE("a message view reads through; copies and writes own their data", "[unit][message]")
{
	des::tag a = des::tag_registry::define("unit_test_a");
	des::tag extra = des::tag_registry::define("unit_test_extra");
	des::tag_store fields;
	fields.set(a, 1.0);
	fields.set(EVENT_CLS, 2.0);
	des::message view = des::message::view_of(fields);
	des::message kept = view;                  // an observer keeping the message
	fields.set(a, 5.0);
	CHECK_EQ(view.get_value(a), 5.0);
	CHECK_EQ(kept.get_value(a), 1.0);
	view.add(extra, 3.0);                      // writing detaches the view
	fields.set(a, 7.0);
	CHECK_EQ(view.get_value(a), 5.0);
	CHECK_EQ(view.get_value(extra), 3.0);
	CHECK(!fields.has(extra));
	view.remove(a);
	CHECK(!view.has(a));
	CHECK(fields.has(a));
	CHECK_EQ(view.get_value(a), 0.0);
}

TEST_CASE("serialize names the fields after the registry", "[unit][message]")
{
	des::tag a = des::tag_registry::define("unit_test_a");
	des::message m;
	m.add(a, 7.0);
	m.add(NODE_WAIT, 0.5);
	std::string s = m.serialize();
	CHECK(s.find("unit_test_a,7.000000;") != std::string::npos);
	CHECK(s.find("node_wait,0.500000;") != std::string::npos);
}
