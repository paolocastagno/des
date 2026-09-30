#ifndef STORE_H
#define STORE_H

#include <functional>
#include <memory>
#include <vector>

#include "libdes_event.hpp"

using namespace std;

namespace des
{
	class job_store;
	class sequence_store;
	class time_store;
}

/**
 * @brief Jobs held by one des::queue, released in the order chosen by the des::policy
 * that created the store (see policy::make_store()).
 *
 * The container and its invariants are private to each implementation: a queue only
 * uses this interface. To add a queueing discipline, derive a store from this class
 * (or from one of the stores below) and return it from your policy's make_store().
 */
class des::job_store
{
	public:
		virtual ~job_store() {}
		/**
		 * @brief Adds job @p e at simulation time @p now
		 */
		virtual void push(const shared_ptr<event>& e, double now) = 0;
		/**
		 * @brief Returns the job released next. The store must not be empty.
		 */
		virtual const shared_ptr<event>& next() const = 0;
		/**
		 * @brief Removes and returns the job released next, at simulation time @p now.
		 * The store must not be empty.
		 */
		virtual shared_ptr<event> pop(double now) = 0;
		/**
		 * @brief Removes and returns the first job, in release order, satisfying @p eligible
		 *
		 * @return the job, or nullptr if no job qualifies
		 */
		virtual shared_ptr<event> pop_first(const function<bool(const event&)>& eligible, double now) = 0;
		/**
		 * @brief Tells whether some job satisfies @p eligible
		 */
		virtual bool has(const function<bool(const event&)>& eligible) const = 0;
		/**
		 * @brief Returns the number of jobs held
		 */
		virtual size_t size() const = 0;
		/**
		 * @brief Calls @p f on every job, in any order. @p f may change the jobs' times:
		 * the store restores its order afterwards.
		 */
		virtual void for_each(const function<void(event&)>& f) = 0;
		/**
		 * @brief Removes all the jobs
		 */
		virtual void clear() = 0;
};

/**
 * @brief Jobs kept in time order (ties in arrival order), released from the earliest end
 * (first-in first-out) or from the latest end (last-in first-out).
 *
 * Jobs that arrive in time order, as in waiting queues, are appended in O(1).
 */
class des::sequence_store : public des::job_store
{
	public:
		/**
		 * @param release_front true to release the earliest job first (FIFO), false the latest (LIFO)
		 */
		explicit sequence_store(bool release_front = true) : front(release_front)
		{}
		void push(const shared_ptr<event>& e, double now) override;
		const shared_ptr<event>& next() const override;
		shared_ptr<event> pop(double now) override;
		shared_ptr<event> pop_first(const function<bool(const event&)>& eligible, double now) override;
		bool has(const function<bool(const event&)>& eligible) const override;
		size_t size() const override;
		void for_each(const function<void(event&)>& f) override;
		void clear() override;
	private:
		bool front;
		/**
		 * @brief Ring buffer: job i, in time order, is ring[(head + i) & (ring.size() - 1)];
		 *        its size is zero or a power of two
		 */
		vector<shared_ptr<event>> ring;
		size_t head = 0;
		size_t count = 0;
		inline shared_ptr<event>& at(size_t i)
		{
			return ring[(head + i) & (ring.size() - 1)];
		}
		inline const shared_ptr<event>& at(size_t i) const
		{
			return ring[(head + i) & (ring.size() - 1)];
		}
		/**
		 * @brief Index, in time order, of the first job in release order satisfying @p eligible, count if none
		 */
		size_t find(const function<bool(const event&)>& eligible) const;
		/**
		 * @brief Double the room, keeping the jobs in order
		 */
		void grow();
};

/**
 * @brief Jobs released in order of time, earliest first (ties in arrival order).
 *
 * Kept in a binary heap: O(log n) insertion and removal, suited to servers holding
 * many jobs at once (infinite server, processor sharing).
 */
class des::time_store : public des::job_store
{
	public:
		void push(const shared_ptr<event>& e, double now) override;
		const shared_ptr<event>& next() const override;
		shared_ptr<event> pop(double now) override;
		shared_ptr<event> pop_first(const function<bool(const event&)>& eligible, double now) override;
		bool has(const function<bool(const event&)>& eligible) const override;
		size_t size() const override;
		void for_each(const function<void(event&)>& f) override;
		void clear() override;
	protected:
		struct entry
		{
			double time;              ///< the job's time, cached so comparisons do not reach the event
			unsigned long long seq;   ///< arrival order, breaks ties between equal times
			shared_ptr<event> job;
		};
		/**
		 * @brief Heap of the jobs: jobs[0] is released next
		 */
		vector<entry> jobs;
		/**
		 * @brief Tells whether @p a is released before @p b
		 */
		static inline bool before(const entry& a, const entry& b)
		{
			return a.time < b.time || (a.time == b.time && a.seq < b.seq);
		}
		/**
		 * @brief Removes and returns the job at heap position @p i
		 */
		shared_ptr<event> remove_at(size_t i);
	private:
		unsigned long long seq = 0;
		void sift_up(size_t i);
		void sift_down(size_t i);
};

#endif
