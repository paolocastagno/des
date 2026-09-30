#ifndef QUEUE_H
#define QUEUE_H

#include <algorithm>
#include <functional>
#include <memory>
#include <limits>

#include "libdes_object.hpp"
#include "libdes_event.hpp"
#include "libdes_policy.hpp"
#include "libdes_store.hpp"

using namespace std;

namespace des
{
	class queue;
}

/**
 * @brief A bounded set of jobs, admitted and released according to a des::policy.
 *
 * The jobs are held by a des::job_store created by the policy, so the queue does not
 * depend on how a discipline stores or orders them.
 */
class des::queue : public des::object
{
	friend class node;
	template <typename , template <typename> typename> friend class station;
    public:
        /**
		 * @brief Creates a new queue object with unlimited positions
		 *
         * @param pol policy employed to handle the queue
		 * @throws invalid_argument if @p pol is null
		 */
        queue(shared_ptr<policy> pol);
        /**
		 * @brief Creates a new queue object
		 *
		 * @param positions queue size
		 * @param pol policy employed to handle the queue
		 * @throws invalid_argument if @p pol is null
		 */
        queue(int positions, shared_ptr<policy> pol);
		/**
		 * @brief Creates a new queue object
		 *
		 * @param positions queue size
		 * @param pol policy employed to handle the queue
		 * @throws invalid_argument if @p pol is null
		 */
        queue(unsigned int positions, shared_ptr<policy> pol);
		/**
		 * @brief Destroy the queue object
		 *
		 */
        ~queue(){}
		/**
		 * @brief Returns the number of events currently stored in the queue.
		 */
		inline int size() const
		{
			return static_cast<int>(held);
		}
		/**
		 * @brief
		 *
		 * @return string
		 */
		string to_string() const override;
	private:
        /**
		 * @brief Adds job @p e at time @p time, if the policy admits it
		 *
		 * @return true if the job was admitted
		 */
        bool enqueue(const shared_ptr<event>& e, double time);
        /**
		 * @brief Dequeues the job released next
		 *
         * @return the dequeued job
		 *
		 */
        shared_ptr<event> dequeue();
        /**
		 * @brief Dequeues the job released next at time @p time. Time-aware stores (e.g.
		 *        processor sharing) update the jobs that stay in the queue.
		 *
		 * @param time current simulation time (departure time of the leaving job)
         * @return the dequeued job
		 */
        shared_ptr<event> dequeue(double time);
        /**
		 * @brief Dequeues the first job, in release order, satisfying @p eligible
		 *
		 * @param eligible predicate on the job
		 * @param time current simulation time
         * @return the dequeued job, or nullptr if no job qualifies
		 */
        shared_ptr<event> dequeue_next(const function<bool(const event&)>& eligible, double time);
        /**
		 * @brief Inspects whether the queue holds a job satisfying @p eligible
		 */
        bool has_next(const function<bool(const event&)>& eligible) const;
        // Utility methods
        /**
        * @brief Returns the time of the job released next
        *
        * @return its time, or __DBL_MAX__ if the queue is empty
        */
        inline double min_time() const
        {
            return held > 0 ? jobs -> next() -> get_time() : __DBL_MAX__;
        }
        /**
		 * @brief Inspects whether the queue is full or not
		 *
		 * @return whether the queue is full or not
		 *
		 */
        inline bool is_full() const
        {
            return held >= pos;
        }
        /**
		 * @brief Inspects the number of places in use
		 *
		 * @return the number of places in use
		 *
		 */
        inline int in_queue() const
        {
            return static_cast<int>(held);
        }
        /**
		 * @brief reset the events' happening time according to a modification of the global time
		 *
		 * @param delta time to add all events happening time
		 *
		 */
        void reset(double time, vector<tag> keys = vector<tag>(), bool newrun = false) override;
        /**
		 * @brief removes all elmeents in the queue
		 *
		 */
        void clear() override;
        // Get and Set methods
        /**
		 * @brief Inspects the number of positions in the queue
		 *
		 */
        int get_positions() const;
        /**
		 * @brief Sets the number of positions in the queue
         *
         * @param positions the number of positions
		 *
		 */
        void set_positions(int positions);
        /**
		 * @brief Inspect the policy name
		 *
         *
         * @return A string representing the policy name
		 */
        string get_policy() const;
        /**
		 * @brief Sets the policy to hanlde the queue
		 *
         * @param pol policy employed to handle the queue
		 * @throws logic_error if the queue is not empty
		 */
        void set_policy(policy* pol);

        unsigned int pos;
        shared_ptr<policy> p;
        unique_ptr<job_store> jobs;
        size_t held = 0;   ///< number of jobs in the store, kept by the methods that change it
		inline static atomic<unsigned int> id_gen{0};
};

#endif
