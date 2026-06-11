#ifndef SOURCESINK_H
#define SOURCESINK_H

#include "libdes_node.hpp"
#include "libdes_event.hpp"
#include "libdes_fifo.hpp"
#include "libdes_queue.hpp"
#include "libdes_const.hpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <random>
#include <memory>

using namespace std;

namespace des
{
	class sourcesink;
}

/**
 * @brief Shared base for external source and terminal sink nodes.
 *
 * sourcesink keeps the common node behavior used by source and sink and owns a
 * process-wide pool of reusable event objects. Sinks return completed events to
 * the pool with dispose_event(); sources take events back with get_event() when
 * creating the next external arrival.
 */
class des::sourcesink : public des::node
{
	public:
		/**
		 * @brief Construct a one-class source/sink base node.
		 *
		 * @param description String identifier.
		 */
		sourcesink(string description) : node::node(description)
		{}

		/**
		 * @brief Construct a source/sink base node for multiple event classes.
		 *
		 * @param description String identifier.
		 * @param cls Number of event classes.
		 */
		sourcesink(string description, int cls) : node::node(description, cls){};
		/**
		 * @brief Construct a one-class source/sink base node with a generator.
		 *
		 * @param description String identifier.
		 * @param gen Shared pseudo-random generator.
		 */
		sourcesink(string description, shared_ptr<mt19937_64> gen) : node::node(description, gen){};
		/**
		 * @brief Construct a source/sink base node with classes and a generator.
		 *
		 * @param description String identifier.
		 * @param cls Number of event classes.
		 * @param gen Shared pseudo-random generator.
		 */
		sourcesink(string description, int cls, shared_ptr<mt19937_64> gen) : node::node(description, cls, gen){};
		/**
		 * @brief Destroy the source/sink base.
		 */
		virtual ~sourcesink() {}

		// Utility methods
		/**
		 * @brief Get a cleared event object from the reusable source/sink pool.
		 *
		 * Allocates a new event only when the pool is empty. Reused events are
		 * cleared before being returned.
		 *
		 * @return Event ready to be initialized for a new source arrival.
		 */
		shared_ptr<event> get_event()
		{
			if(events.empty())
			{
				return shared_ptr<event>(new event());
			}
			else
			{
				shared_ptr<event> ret = *events.begin();
				events.pop_front();
				ret -> clear();
				return ret;
			}
		}
		/**
		 * @brief Return an event to the reusable source/sink pool.
		 *
		 * Sinks call this when a routed event reaches the terminal node.
		 *
		 * @param e Event to recycle.
		 */
		void dispose_event(shared_ptr<event> e)
		{
			events.push_back(e);
		}
		/**
		 * @brief Reset the base node and attached observers.
		 *
		 * This is a thin delegate to node::reset(). Passing @p newrun through is
		 * important because observers use it to snapshot completed-run values.
		 *
		 * @param value Elapsed simulation time to subtract from pending events.
		 * @param keys Additional event-info keys whose times should be shifted.
		 * @param newrun When true, observers snapshot their current-run values.
		 */
		void reset(double value, vector<string> keys = vector<string>(), bool newrun = false) override
		{
			node::reset(value, keys, newrun);
		}
		/**
		 * @brief Clear node state and attached observer state.
		 */
		virtual void clear() override
		{
			node::clear();
		}
	private:
		inline static list<shared_ptr<event>> events = list<shared_ptr<event>>(); ///< Shared pool of reusable terminal/source events.
};

#endif
