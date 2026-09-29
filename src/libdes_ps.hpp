#ifndef PS_H
#define PS_H

#include <memory>
#include <utility>
#include <vector>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"
#include "libdes_store.hpp"

using namespace std;

namespace des
{
	class ps;
	class ps_store;
}

/**
 * @brief Processor Sharing (PS) / Generalized Processor Sharing (GPS) policy.
 *
 * All jobs in the server share the CPU simultaneously.  Each job class c
 * carries a positive weight w_c; with total weight W = Σ w_{c_k} over all
 * jobs currently in service, a job of class c is served at rate w_c / W.
 * When all weights are equal (default) this reduces to standard PS.
 *
 * The jobs are held by a des::ps_store.
 *
 * ### Usage
 * Attach this policy to the *server* queue of a station with unlimited
 * capacity so that all arriving jobs go directly into service.
 * No separate waiting queue is needed.
 *
 * ### Constructors
 *   ps()                        — standard PS (all weights = 1)
 *   ps(vector<double> weights)  — GPS: weights[c] is the weight for class c
 *                                 Classes beyond the vector size default to 1.
 */
class des::ps : public des::policy
{
	public:
		/** Standard PS: all job classes share the CPU equally. */
		ps();
		/**
		 * @brief GPS: each class gets CPU proportional to its weight.
		 *
		 * @param weights  weights[c] > 0 is the weight for class c.
		 *                 Classes with index >= weights.size() default to 1.
		 * @throws invalid_argument if a weight is not a positive finite number
		 */
		explicit ps(vector<double> weights);
		/**
		 * @brief Creates a des::ps_store with this policy's weights
		 */
		unique_ptr<job_store> make_store() const override;

	private:
		vector<double> w; ///< per-class weights; empty means all weights = 1
};

/**
 * @brief Jobs sharing a processor, tracked in virtual time.
 *
 * The virtual time V advances at rate 1 / W(t), where W(t) is the total weight of the
 * jobs held. A job of class c arriving at time a with service requirement s receives
 * the virtual finish tag
 *
 *   F = V(a) + s / w_c
 *
 * and departs when V reaches F. Tags never change, so jobs are kept in a heap ordered
 * by F: arrivals and departures cost O(log n) and no departure time is ever rescaled,
 * so no rounding error accumulates. Between two events W is constant, and the next
 * departure happens at
 *
 *   t = t_V + (F_min - V) * W
 *
 * where t_V is the real time at which V was last updated. Only the next job's event
 * time is kept up to date; for_each() sets the other jobs' times to the time they would
 * depart if the jobs held did not change.
 *
 * The time of a job pushed at time now must be now + its service requirement when
 * served alone, as set by des::node.
 */
class des::ps_store : public des::job_store
{
	public:
		/**
		 * @param weights  weights[c] > 0 is the weight for class c; classes beyond the vector size weigh 1
		 */
		explicit ps_store(vector<double> weights);
		void push(const shared_ptr<event>& e, double now) override;
		const shared_ptr<event>& next() const override;
		shared_ptr<event> pop(double now) override;
		shared_ptr<event> pop_first(const function<bool(const event&)>& eligible, double now) override;
		bool has(const function<bool(const event&)>& eligible) const override;
		size_t size() const override;
		/**
		 * @brief Sets every job's time to the time it would depart if the jobs held did not
		 * change, calls @p f on each job, then resumes from the times @p f left.
		 */
		void for_each(const function<void(event&)>& f) override;
		void clear() override;

	private:
		struct entry
		{
			double finish;            ///< virtual finish tag F
			unsigned long long seq;   ///< arrival order, breaks ties between equal tags
			shared_ptr<event> job;
		};
		vector<entry> jobs;              ///< heap: jobs[0] departs next
		vector<double> w;                ///< per-class weights; empty means all weights = 1
		vector<unsigned long> in_class;  ///< number of jobs held per class
		double total = 0.0;              ///< W: total weight of the jobs held
		double vtime = 0.0;              ///< V at real time `anchor`
		double anchor = 0.0;             ///< real time of the last update of `vtime`
		unsigned long long seq = 0;

		/** Heap order: tells whether @p a departs after @p b. */
		static inline bool after(const entry& a, const entry& b)
		{
			return a.finish > b.finish || (a.finish == b.finish && a.seq > b.seq);
		}
		/** Returns the weight of class @p cls (defaults to 1 if it has none). */
		double weight(int cls) const;
		/** Advances the virtual time to real time @p now. */
		void advance(double now);
		/** Adds @p delta jobs of class @p cls and recomputes W from the per-class counts. */
		void count(int cls, long delta);
		/** Returns the real departure time of @p en if the jobs held do not change. */
		inline double departure(const entry& en) const
		{
			return anchor + (en.finish - vtime) * total;
		}
		/** Sets the next job's event time to its departure time. */
		void schedule_next();
		/** Bookkeeping after job @p e left at time @p now. */
		void departed(const shared_ptr<event>& e, double now);
};
#endif
