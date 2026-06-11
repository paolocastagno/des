#ifndef PS_H
#define PS_H

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class ps;
}

/**
 * @brief Processor Sharing (PS) / Generalized Processor Sharing (GPS) policy.
 *
 * All jobs in the server share the CPU simultaneously.  Each job class c
 * carries a positive weight w_c; with total weight W = Σ w_{c_k} over all
 * jobs currently in service, a job of class c is served at rate w_c / W.
 * When all weights are equal (default) this reduces to standard PS.
 *
 * ### Invariant
 * For every job j of class c_j present at time t (n jobs, total weight W):
 *
 *   event_time(j) = t + r_j * W / w_{c_j}
 *
 * where r_j is the *equivalent remaining service time* — the remaining work
 * job j would need if it were alone.
 *
 * ### On arrival of class c_new (weight w_new), service time s_new:
 *   W' = W + w_new
 *   - For every existing job j:  d_j'  = t + (d_j  - t) * W' / W
 *   - For the new job:           d_new = t + s_new  * W' / w_new
 *
 *   The scale factor W'/W is class-independent, so the sorted order of
 *   existing jobs is preserved and no list reordering is triggered.
 *
 * ### On departure of class c_dep (weight w_dep) — via on_dequeue():
 *   W' = W - w_dep
 *   - For every remaining job j: d_j' = t + (d_j - t) * W' / W
 *
 *   Again class-independent — sorted order preserved.
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
		 */
		explicit ps(vector<double> weights);

		/**
		 * @brief Admits the job and rescales all departure times for the new
		 *        total weight.  Returns false only when the server is full.
		 */
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double time) override;
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions) override;
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, double rate) override;

		/** Always dequeues from the front (smallest departure time). */
		bool front() override;

		/**
		 * @brief Rescales remaining jobs' departure times after a departure.
		 *        Called by queue::dequeue(time) *before* the front is removed.
		 */
		void on_dequeue(list<shared_ptr<event>>& l, double time) override;

	private:
		vector<double> w; ///< per-class weights; empty means all weights = 1

		/** Returns the weight for class cls (defaults to 1 if not set). */
		double get_weight(unsigned int cls) const;

		/** Returns the total weight of all jobs currently in the list. */
		double total_weight(const list<shared_ptr<event>>& l) const;
};
#endif
