#include "libdes_store.hpp"

#include <algorithm>
#include <iterator>

namespace des
{
	/* ~~~~~~~~~~~~~~ *
	*  sequence_store *
	* ~~~~~~~~~~~~~~~ *
	*/
	void sequence_store::push(const shared_ptr<event>& e, double)
	{
		if(count == ring.size())
		{
			grow();
		}
		// Insert after the last job not later than e: O(1) when jobs arrive in time order
		double t = e -> get_time();
		size_t i = count;
		while(i > 0 && t < at(i - 1) -> get_time())
		{
			at(i) = std::move(at(i - 1));
			--i;
		}
		at(i) = e;
		++count;
	}

	const shared_ptr<event>& sequence_store::next() const
	{
		return front ? at(0) : at(count - 1);
	}

	shared_ptr<event> sequence_store::pop(double)
	{
		shared_ptr<event> ret;
		if(front)
		{
			ret = std::move(at(0));
			head = (head + 1) & (ring.size() - 1);
		}
		else
		{
			ret = std::move(at(count - 1));
		}
		--count;
		return ret;
	}

	size_t sequence_store::find(const function<bool(const event&)>& eligible) const
	{
		for(size_t k = 0; k < count; k++)
		{
			size_t i = front ? k : count - 1 - k;
			if(eligible(*at(i)))
			{
				return i;
			}
		}
		return count;
	}

	shared_ptr<event> sequence_store::pop_first(const function<bool(const event&)>& eligible, double now)
	{
		size_t i = find(eligible);
		if(i == count)
		{
			return nullptr;
		}
		if(front && i == 0)
		{
			return pop(now);
		}
		shared_ptr<event> ret = std::move(at(i));
		for(; i + 1 < count; i++)
		{
			at(i) = std::move(at(i + 1));
		}
		--count;
		return ret;
	}

	bool sequence_store::has(const function<bool(const event&)>& eligible) const
	{
		return find(eligible) != count;
	}

	size_t sequence_store::size() const
	{
		return count;
	}

	void sequence_store::for_each(const function<void(event&)>& f)
	{
		vector<shared_ptr<event>> jobs;
		jobs.reserve(count);
		for(size_t i = 0; i < count; i++)
		{
			f(*at(i));
			jobs.push_back(std::move(at(i)));
		}
		// Restore the time order, keeping the arrival order of ties
		auto earlier = [](const shared_ptr<event>& a, const shared_ptr<event>& b){ return a -> get_time() < b -> get_time(); };
		if(!is_sorted(jobs.begin(), jobs.end(), earlier))
		{
			stable_sort(jobs.begin(), jobs.end(), earlier);
		}
		head = 0;
		for(size_t i = 0; i < count; i++)
		{
			ring[i] = std::move(jobs[i]);
		}
	}

	void sequence_store::clear()
	{
		for(size_t i = 0; i < count; i++)
		{
			at(i).reset();
		}
		head = 0;
		count = 0;
	}

	void sequence_store::grow()
	{
		vector<shared_ptr<event>> bigger(ring.empty() ? 4 : 2 * ring.size());
		for(size_t i = 0; i < count; i++)
		{
			bigger[i] = std::move(at(i));
		}
		ring = std::move(bigger);
		head = 0;
	}

	/* ~~~~~~~~~~ *
	*  time_store *
	* ~~~~~~~~~~~ *
	*/
	void time_store::push(const shared_ptr<event>& e, double)
	{
		jobs.push_back(entry{e -> get_time(), seq++, e});
		sift_up(jobs.size() - 1);
	}

	const shared_ptr<event>& time_store::next() const
	{
		return jobs.front().job;
	}

	shared_ptr<event> time_store::pop(double)
	{
		return remove_at(0);
	}

	shared_ptr<event> time_store::pop_first(const function<bool(const event&)>& eligible, double)
	{
		// The heap is not sorted: look for the earliest eligible job among all of them
		size_t best = jobs.size();
		for(size_t i = 0; i < jobs.size(); i++)
		{
			if(eligible(*jobs[i].job) && (best == jobs.size() || before(jobs[i], jobs[best])))
			{
				best = i;
			}
		}
		if(best == jobs.size())
		{
			return nullptr;
		}
		return remove_at(best);
	}

	bool time_store::has(const function<bool(const event&)>& eligible) const
	{
		return any_of(jobs.begin(), jobs.end(), [&eligible](const entry& en){ return eligible(*en.job); });
	}

	size_t time_store::size() const
	{
		return jobs.size();
	}

	void time_store::for_each(const function<void(event&)>& f)
	{
		for(entry& en: jobs)
		{
			f(*en.job);
			en.time = en.job -> get_time();
		}
		// Rebuild the heap
		for(size_t i = jobs.size() / 2; i-- > 0;)
		{
			sift_down(i);
		}
	}

	void time_store::clear()
	{
		jobs.clear();
		seq = 0;
	}

	shared_ptr<event> time_store::remove_at(size_t i)
	{
		shared_ptr<event> ret = std::move(jobs[i].job);
		size_t last = jobs.size() - 1;
		if(i != last)
		{
			jobs[i] = std::move(jobs[last]);
		}
		jobs.pop_back();
		if(i < jobs.size())
		{
			sift_up(i);
			sift_down(i);
		}
		return ret;
	}

	void time_store::sift_up(size_t i)
	{
		while(i > 0)
		{
			size_t parent = (i - 1) / 2;
			if(!before(jobs[i], jobs[parent]))
			{
				break;
			}
			swap(jobs[i], jobs[parent]);
			i = parent;
		}
	}

	void time_store::sift_down(size_t i)
	{
		size_t n = jobs.size();
		while(true)
		{
			size_t first = i, l = 2 * i + 1, r = l + 1;
			if(l < n && before(jobs[l], jobs[first]))
			{
				first = l;
			}
			if(r < n && before(jobs[r], jobs[first]))
			{
				first = r;
			}
			if(first == i)
			{
				break;
			}
			swap(jobs[i], jobs[first]);
			i = first;
		}
	}
}
