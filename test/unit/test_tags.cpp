#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_tag.hpp>

#include "des_test.hpp"

TEST_CASE("reserved tags alias the constants and carry their registry names", "[unit][tag]")
{
	const std::vector<std::pair<des::tag, std::string>> builtins = {
		{EVENT_ID, "id"}, {EVENT_CLS, "class"}, {EVENT_TIME, "time"}, {EVENT_CONSTRAINT, "constraint"},
		{EVENT_REROUTE, "reroute"}, {EVENT_NODE, "node"}, {EVENT_QUEUE, "queue_idx"}, {EVENT_SERVER, "server_idx"},
		{EVENT_REJECT, "reject"}, {NODE_ARRIVAL, "arrival_time"}, {NODE_SERVICE_START, "service_start_time"},
		{NODE_SOJOURN, "node_sojourn"}, {NODE_WAIT, "node_wait"}, {NODE_SERVICE, "node_service"}};
	REQUIRE(builtins.size() == des::tags::BUILTIN_COUNT);
	for(size_t i = 0; i < builtins.size(); i++)
	{
		const auto& [t, name] = builtins[i];
		CHECK_EQ(t.id, i);
		CHECK(des::tag_registry::is_builtin(t));
		CHECK_EQ(des::tag_registry::name(t), name);
		auto found = des::tag_registry::find(name);
		CHECK(found.first && found.second == t);
	}
}

TEST_CASE("reserved names cannot be defined as user tags", "[unit][tag]")
{
	CHECK_THROWS_AS(std::invalid_argument, des::tag_registry::define("time"));
	CHECK_THROWS_AS(std::invalid_argument, des::tag_registry::define("node_sojourn"));
}

TEST_CASE("define is idempotent and returns non-reserved tags", "[unit][tag]")
{
	des::tag a = des::tag_registry::define("unit_test_priority");
	CHECK(!des::tag_registry::is_builtin(a));
	CHECK(des::tag_registry::define("unit_test_priority") == a);
	CHECK_EQ(des::tag_registry::name(a), std::string("unit_test_priority"));
	CHECK(des::tag_registry::find("unit_test_priority").second == a);
	CHECK(!des::tag_registry::find("unit_test_never_defined").first);
	CHECK_THROWS_AS(std::out_of_range, des::tag_registry::name(des::tag{1000000}));
}

TEST_CASE("tag_store sets, inserts, removes and visits fields in tag order", "[unit][tag]")
{
	des::tag_store s;
	des::tag far = des::tag{40};
	CHECK(!s.has(EVENT_NODE));
	CHECK_EQ(s.value(EVENT_NODE), 0.0);
	s.set(far, 2.0);                 // grows on demand
	s.set(EVENT_NODE, 1.0);
	CHECK(s.has(far) && s.get(far).second == 2.0);
	CHECK(!s.insert(EVENT_NODE, 5.0));
	CHECK_EQ(s.value(EVENT_NODE), 1.0);
	CHECK(s.insert(EVENT_QUEUE, 3.0));
	std::vector<unsigned int> order;
	s.for_each([&order](des::tag t, double){ order.push_back(t.id); });
	CHECK((order == std::vector<unsigned int>{EVENT_NODE.id, EVENT_QUEUE.id, far.id}));
	s.remove(EVENT_NODE);
	CHECK(!s.has(EVENT_NODE));
	s.clear();
	CHECK(!s.has(far) && !s.has(EVENT_QUEUE));
}

TEST_CASE("registry is safe under concurrent define/find/name", "[unit][threads][tag]")
{
	std::vector<std::thread> threads;
	for(int k = 0; k < 4; k++)
	{
		threads.emplace_back([]{
			for(int i = 0; i < 2000; i++)
			{
				std::string n = "unit_test_field_" + std::to_string(i % 300);
				des::tag t = des::tag_registry::define(n);
				CHECK_EQ(des::tag_registry::name(t), n);
				CHECK(des::tag_registry::find(n).second == t);
			}
		});
	}
	for(std::thread& t : threads) t.join();
	CHECK(des::tag_registry::find("unit_test_field_299").first);
}
