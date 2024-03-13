#ifndef QUEUE_H
#define QUEUE_H

#include <list> 
#include <memory>
#include <algorithm>
#include <limits>

#include "libdes_object.hpp"
#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class queue;
}

class des::queue : public des::object
{
	friend class node;
	template <typename , template <typename> typename> friend class station;
    public:
        // /**
		//  * @brief Creates a new empty queue object
		//  */
        queue();
        /**
		 * @brief Creates a new queue object
		 * 
		 * @param positions queue size
         * @param p policy employed to handle te queue
		 * 
		 */
        queue(shared_ptr<policy> pol);
        /**
		 * @brief Creates a new queue object
		 * 
		 * @param p policy employed to handle the queue
		 * 
		 */
        queue(int positions, shared_ptr<policy> pol);
		/**
		 * @brief Creates a new queue object
		 * 
		 * @param p policy employed to handle the queue
		 * 
		 */
        queue(unsigned int positions, shared_ptr<policy> pol);
		/**
		 * @brief Destroy the queue object
		 * 
		 */
        ~queue(){}
		/**
		 * @brief 
		 * 
		 * @return string 
		 */
		string to_string() const override;
	private:
        // Handle insertions and deletions
        bool enqueue(shared_ptr<event> e, double time);
        /**
		 * @brief Dequeues one job
		 * 
         * @return the dequeued job
		 * 
		 */
        shared_ptr<event> dequeue();
        // Utility methods
        /**
        * @brief Returns the minimum time at which an event has happened
        *
        * @return minimum time
        */
        double min_time() const;
		/**
		 * @brief Inserts an element in the list in increasing order
		 * 
		 * @param e 
		 */
		void insert(shared_ptr<event> e);
        /**
		 * @brief Inspects whether the queue is full or not
		 * 
		 * @return whether the queue is full or not
		 * 
		 */
        bool is_full() const;
        /**
		 * @brief Inspects the number of places in use
		 * 
		 * @return the number of places in use
		 * 
		 */
        int in_queue() const;
        /**
		 * @brief reset the events' happening time according to a modification of the global time 
		 * 
		 * @param delta time to add all events happening time
		 * 
		 */
        void reset(double time, vector<string> keys = vector<string>(), bool newrun = false) override;
        /**
		 * @brief removes all elmeents in the queue and clears them
		 * 
		 * @param empty structure to host empty events, or nullptr
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
         * @return the number of positions
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
         * @param p policy employed to handle the queue
		 * 
		 */
        void set_policy(policy* pol);

        unsigned int pos;
        list<shared_ptr<event>> lst;
        shared_ptr<policy> p;
		inline static unsigned int id_gen = 0;
};

#endif