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
		// Insert after the last job not later than e: O(1) when jobs arrive in time order
		double t = e -> get_time();
		deque<shared_ptr<event>>::iterator it = jobs.end();
		while(it != jobs.begin() && t < (*prev(it)) -> get_time())
		{
			--it;
		}
		jobs.insert(it, e);
	}

	const shared_ptr<event>& sequence_store::next() const
	{
		return front ? jobs.front() : jobs.back();
	}

	shared_ptr<event> sequence_store::pop(double)
	{
		shared_ptr<event> ret;
		if(front)
		{
			ret = std::move(jobs.front());
			jobs.pop_front();
		}
		else
		{
			ret = std::move(jobs.back());
			jobs.pop_back();
		}
		return ret;
	}

	shared_ptr<event> sequence_store::pop_first(const function<bool(const event&)>& eligible, double)
	{
		auto match = [&eligible](const shared_ptr<event>& e){ return eligible(*e); };
		deque<shared_ptr<event>>::iterator it;
		if(front)
		{
			it = find_if(jobs.begin(), jobs.end(), match);
		}
		else
		{
			deque<shared_ptr<event>>::reverse_iterator rit = find_if(jobs.rbegin(), jobs.rend(), match);
			it = rit == jobs.rend() ? jobs.end() : prev(rit.base());
		}
		if(it == jobs.end())
		{
			return nullptr;
		}
		shared_ptr<event> ret = std::move(*it);
		jobs.erase(it);
		return ret;
	}

	bool sequence_store::has(const function<bool(const event&)>& eligible) const
	{
		return any_of(jobs.begin(), jobs.end(), [&eligible](const shared_ptr<event>& e){ return eligible(*e); });
	}

	size_t sequence_store::size() const
	{
		return jobs.size();
	}

	void sequence_store::for_each(const function<void(event&)>& f)
	{
		for(shared_ptr<event>& e: jobs)
		{
			f(*e);
		}
		// Restore the time order, keeping the arrival order of ties
		auto earlier = [](const shared_ptr<event>& a, const shared_ptr<event>& b){ return a -> get_time() < b -> get_time(); };
		if(!is_sorted(jobs.begin(), jobs.end(), earlier))
		{
			stable_sort(jobs.begin(), jobs.end(), earlier);
		}
	}

	void sequence_store::clear()
	{
		jobs.clear();
	}

	/* ~~~~~~~~~~ *
	*  time_store *
	* ~~~~~~~~~~~ *
	*/
	void time_store::push(const shared_ptr<event>& e, double)
	{
		jobs.push_back(entry{seq++, e});
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
