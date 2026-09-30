#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_fifo.hpp>
#include <libdes_is.hpp>
#include <libdes_ps.hpp>
#include <libdes_queue.hpp>
#include <libdes_store.hpp>

#include "des_models.hpp"
#include "des_test.hpp"

using des_models::make_event;

namespace
{
	std::shared_ptr<des::event> job(double t, int cls = 0)
	{
		return make_event(cls, t);
	}

	std::vector<std::shared_ptr<des::event>> drain(des::job_store& s)
	{
		std::vector<std::shared_ptr<des::event>> out;
		while(s.size() != 0) out.push_back(s.pop(s.next() -> get_time()));
		return out;
	}
}

TEST_CASE("sequence_store releases in time order, ties in arrival order", "[unit][store]")
{
	des::sequence_store fifo(true);
	auto a = job(2.0), b = job(1.0), c = job(2.0), d = job(1.0);
	for(auto& e : {a, b, c, d}) fifo.push(e, 0.0);
	CHECK((drain(fifo) == std::vector<std::shared_ptr<des::event>>{b, d, a, c}));

	des::sequence_store lifo(false);
	for(auto& e : {a, b, c, d}) lifo.push(e, 0.0);
	CHECK((drain(lifo) == std::vector<std::shared_ptr<des::event>>{c, a, d, b}));
}

TEST_CASE("time_store releases the earliest job, ties in arrival order", "[unit][store]")
{
	des::time_store s;
	std::vector<std::shared_ptr<des::event>> in;
	for(int i = 0; i < 20; i++)
	{
		in.push_back(job(i % 2 ? 1.0 : 2.0));
		s.push(in.back(), 0.0);
	}
	auto out = drain(s);
	for(int i = 0; i < 10; i++)
	{
		CHECK(out[i] == in[2 * i + 1]);
		CHECK(out[10 + i] == in[2 * i]);
	}
}

TEST_CASE("pop_first releases the first eligible job in release order", "[unit][store]")
{
	auto odd = [](const des::event& e){ return e.get_cls() == 1; };
	for(bool heap : {false, true})
	{
		std::unique_ptr<des::job_store> s;
		if(heap) s = std::make_unique<des::time_store>();
		else s = std::make_unique<des::sequence_store>(true);
		auto a = job(1.0, 0), b = job(2.0, 1), c = job(3.0, 1);
		s -> push(c, 0.0);
		s -> push(a, 0.0);
		s -> push(b, 0.0);
		CHECK(s -> has(odd));
		CHECK(s -> pop_first(odd, 0.0) == b);
		CHECK(s -> pop_first(odd, 0.0) == c);
		CHECK(s -> pop_first(odd, 0.0) == nullptr);
		CHECK(!s -> has(odd));
		CHECK(s -> next() == a);
	}
}

TEST_CASE("for_each lets times change and restores the order", "[unit][store]")
{
	for(bool heap : {false, true})
	{
		std::unique_ptr<des::job_store> s;
		if(heap) s = std::make_unique<des::time_store>();
		else s = std::make_unique<des::sequence_store>(true);
		auto a = job(1.0), b = job(2.0), c = job(3.0);
		for(auto& e : {a, b, c}) s -> push(e, 0.0);
		// Reverse the order of the jobs
		s -> for_each([](des::event& e){ e.set_time(10.0 - e.get_time()); });
		CHECK(s -> next() == c);
		CHECK((drain(*s) == std::vector<std::shared_ptr<des::event>>{c, b, a}));
	}
}

TEST_CASE("sequence_store matches a reference model through growth and wrap-around", "[unit][store]")
{
	// Reference: a vector kept in time order, ties in arrival order
	using jobs_t = std::vector<std::shared_ptr<des::event>>;
	auto odd = [](const des::event& e){ return e.get_cls() == 1; };
	for(bool fifo : {true, false})
	{
		des::sequence_store s(fifo);
		jobs_t ref;
		des::random_engine g(99);
		std::uniform_int_distribution<int> op(0, 99), cls(0, 1), ahead(0, 3);
		double clock = 0.0;
		size_t largest = 0;
		for(int i = 0; i < 20000; i++)
		{
			// Alternate phases that fill the store (growing its room) and drain it (moving its start)
			bool filling = (i / 1000) % 2 == 0;
			int pushes = filling ? 60 : 30, pops = filling ? 20 : 45;
			int o = op(g);
			if(o < pushes)
			{
				// Mostly in time order, sometimes earlier than the last job, with ties
				clock += 0.5 * ahead(g);
				auto e = job(ahead(g) == 0 ? clock - 1.0 : clock, cls(g));
				s.push(e, 0.0);
				auto at = std::upper_bound(ref.begin(), ref.end(), e, [](const std::shared_ptr<des::event>& a, const std::shared_ptr<des::event>& b){ return a -> get_time() < b -> get_time(); });
				ref.insert(at, e);
			}
			else if(o < pushes + pops && !ref.empty())
			{
				auto expected = fifo ? ref.front() : ref.back();
				REQUIRE(s.next() == expected);
				REQUIRE(s.pop(0.0) == expected);
				if(fifo) ref.erase(ref.begin()); else ref.pop_back();
			}
			else if(o < pushes + pops + 12)
			{
				// First eligible job in release order
				long found = -1;
				for(size_t k = 0; k < ref.size() && found < 0; k++)
				{
					size_t idx = fifo ? k : ref.size() - 1 - k;
					if(odd(*ref[idx])) found = static_cast<long>(idx);
				}
				REQUIRE(s.has(odd) == (found >= 0));
				auto got = s.pop_first(odd, 0.0);
				REQUIRE(got == (found >= 0 ? ref[found] : nullptr));
				if(found >= 0) ref.erase(ref.begin() + found);
			}
			else if(o < pushes + pops + 14)
			{
				// Times change out of order: both restore it, keeping the order of ties
				s.for_each([](des::event& e){ e.set_time(std::fmod(e.get_time(), 7.0)); });
				std::stable_sort(ref.begin(), ref.end(), [](const std::shared_ptr<des::event>& a, const std::shared_ptr<des::event>& b){ return a -> get_time() < b -> get_time(); });
			}
			else if(i % 5000 == 4999)
			{
				s.clear();
				ref.clear();
			}
			REQUIRE(s.size() == ref.size());
			largest = std::max(largest, ref.size());
		}
		CHECK(largest > 100);
		CHECK((drain(s) == (fifo ? ref : jobs_t(ref.rbegin(), ref.rend()))));
	}
}

TEST_CASE("ps_store: two jobs share the processor", "[unit][store][ps]")
{
	des::ps_store s({});
	// Job a needs 1 time unit alone and arrives at 0; job b needs 1 and arrives at 0.5
	auto a = job(0.0 + 1.0), b = job(0.5 + 1.0);
	s.push(a, 0.0);
	CHECK_EQ(s.next() -> get_time(), 1.0);
	s.push(b, 0.5);
	// At 0.5 a has 0.5 left, served at rate 1/2 -> leaves at 1.5; b then has 0.5 left alone -> 2.0
	REQUIRE(s.next() == a);
	CHECK_NEAR(a -> get_time(), 1.5, 1e-12);
	CHECK(s.pop(1.5) == a);
	REQUIRE(s.next() == b);
	CHECK_NEAR(b -> get_time(), 2.0, 1e-12);
	CHECK(s.pop(2.0) == b);
	CHECK_EQ(s.size(), 0u);
}

TEST_CASE("ps_store: weights split the processor", "[unit][store][ps]")
{
	des::ps_store s({2.0, 1.0});
	// Class 0 (weight 2) and class 1 (weight 1) each need 1 time unit, both arrive at 0
	auto a = job(1.0, 0), b = job(1.0, 1);
	s.push(a, 0.0);
	s.push(b, 0.0);
	// a is served at rate 2/3 -> leaves at 1.5; b has done 0.5, then alone -> 2.0
	REQUIRE(s.next() == a);
	CHECK_NEAR(a -> get_time(), 1.5, 1e-12);
	s.pop(1.5);
	CHECK_NEAR(b -> get_time(), 2.0, 1e-12);
}

TEST_CASE("ps_store: departure times survive a reset shift", "[unit][store][ps]")
{
	des::ps_store s({});
	auto a = job(0.0 + 4.0), b = job(1.0 + 1.0), c = job(1.0 + 3.0);
	s.push(a, 0.0);
	s.push(b, 1.0);
	s.push(c, 1.0);
	// From 1.0 three jobs share: b leaves at 4.0; then a and c (2 left each) both leave at 8.0.
	// Shifted back by 1: 3.0, 7.0, 7.0
	s.for_each([](des::event& e){ e.shift_times(1.0, {}); });
	CHECK(s.next() == b);
	CHECK_NEAR(b -> get_time(), 3.0, 1e-12);
	s.pop(3.0);
	CHECK(s.next() == a);
	CHECK_NEAR(a -> get_time(), 7.0, 1e-12);
	s.pop(7.0);
	CHECK(s.next() == c);
	CHECK_NEAR(c -> get_time(), 7.0, 1e-12);
}

TEST_CASE("ps rejects non-positive weights", "[unit][policy][ps]")
{
	CHECK_THROWS_AS(std::invalid_argument, des::ps({1.0, 0.0}));
	CHECK_THROWS_AS(std::invalid_argument, des::ps({-1.0}));
	CHECK_NOTHROW(des::ps({0.5, 2.0}));
}

TEST_CASE("policies admit by capacity; is admits always", "[unit][policy]")
{
	des::fifo fifo;
	des::is inf;
	auto store = fifo.make_store();
	auto e = job(1.0);
	CHECK(fifo.admit(*e, *store, 1));
	store -> push(e, 0.0);
	CHECK(!fifo.admit(*e, *store, 1));
	CHECK(inf.admit(*e, *store, 0));
}

TEST_CASE("a queue needs a policy", "[unit][policy]")
{
	CHECK_THROWS_AS(std::invalid_argument, des::queue(1u, std::shared_ptr<des::policy>()));
	des::queue q(std::make_shared<des::fifo>());
	CHECK_EQ(q.size(), 0);
}

// ---- disciplines defined outside the library ----

namespace
{
	struct lifo : des::policy
	{
		lifo() : des::policy("lifo") {}
		std::unique_ptr<des::job_store> make_store() const override { return std::make_unique<des::sequence_store>(false); }
	};

	// Higher value of a user field first, ties in arrival order
	class priority_store : public des::job_store
	{
	  public:
		explicit priority_store(des::tag key) : key(key) {}
		void push(const std::shared_ptr<des::event>& e, double) override
		{
			jobs.push_back({e -> get_info(key).second, seq++, e});
			std::push_heap(jobs.begin(), jobs.end(), after);
		}
		const std::shared_ptr<des::event>& next() const override { return jobs.front().job; }
		std::shared_ptr<des::event> pop(double) override
		{
			std::pop_heap(jobs.begin(), jobs.end(), after);
			std::shared_ptr<des::event> e = std::move(jobs.back().job);
			jobs.pop_back();
			return e;
		}
		std::shared_ptr<des::event> pop_first(const std::function<bool(const des::event&)>& eligible, double) override
		{
			auto best = jobs.end();
			for(auto it = jobs.begin(); it != jobs.end(); ++it)
				if(eligible(*it -> job) && (best == jobs.end() || after(*best, *it))) best = it;
			if(best == jobs.end()) return nullptr;
			std::shared_ptr<des::event> e = std::move(best -> job);
			jobs.erase(best);
			std::make_heap(jobs.begin(), jobs.end(), after);
			return e;
		}
		bool has(const std::function<bool(const des::event&)>& eligible) const override
		{
			return std::any_of(jobs.begin(), jobs.end(), [&](const entry& en){ return eligible(*en.job); });
		}
		size_t size() const override { return jobs.size(); }
		void for_each(const std::function<void(des::event&)>& f) override { for(entry& en : jobs) f(*en.job); }
		void clear() override { jobs.clear(); }
	  private:
		struct entry { double prio; unsigned long long seq; std::shared_ptr<des::event> job; };
		static bool after(const entry& a, const entry& b) { return a.prio < b.prio || (a.prio == b.prio && a.seq > b.seq); }
		des::tag key;
		std::vector<entry> jobs;
		unsigned long long seq = 0;
	};

	struct priority : des::policy
	{
		explicit priority(des::tag key) : des::policy("priority"), key(key) {}
		std::unique_ptr<des::job_store> make_store() const override { return std::make_unique<priority_store>(key); }
		des::tag key;
	};

	bool in_service(const std::shared_ptr<des::event>& e)
	{
		return e -> get_info(NODE_SERVICE_START).first;
	}
}

TEST_CASE("a user-defined LIFO policy serves the latest arrival", "[unit][policy]")
{
	using namespace des_models;
	det_station sta(det_dists{{fixed(10)}}, 1, 1, 1, INT_MAX, std::make_shared<lifo>(), std::make_shared<des::fifo>(), "lifo");
	arrive(sta, 0, 0.0);
	auto first = arrive(sta, 0, 1.0);
	arrive(sta, 0, 2.0);
	auto last = arrive(sta, 0, 3.0);
	sta.departure();
	CHECK(in_service(last));
	CHECK(!in_service(first));
	CHECK_EQ(last -> get_info(NODE_SERVICE_START).second, 10.0);
}

TEST_CASE("a user-defined priority policy needs no library change", "[unit][policy]")
{
	using namespace des_models;
	des::tag prio = des::tag_registry::define("unit_test_priority_level");
	det_station sta(det_dists{{fixed(10)}}, 1, 1, 1, INT_MAX, std::make_shared<priority>(prio), std::make_shared<des::fifo>(), "prio");
	auto with_prio = [&](double t, double p)
	{
		auto e = make_event(0, t);
		e -> emplace_info(prio, p);
		REQUIRE(sta.arrival(e));
		return e;
	};
	with_prio(0.0, 0);
	auto low = with_prio(1.0, 1);
	auto high1 = with_prio(2.0, 5);
	auto high2 = with_prio(3.0, 5);
	sta.departure();
	CHECK(in_service(high1));
	CHECK(!in_service(high2));
	sta.departure();
	CHECK(in_service(high2));
	CHECK(!in_service(low));
	sta.departure();
	CHECK(in_service(low));
}
