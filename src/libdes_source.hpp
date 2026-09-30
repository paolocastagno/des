#ifndef SOURCE_H
#define SOURCE_H

#include "libdes_sourcesink.hpp"
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
	template <typename TT, template <typename> typename T> class source;
}

/**
 * @brief Entry node that creates the next external arrival for each departed event.
 *
 * A source has no waiting queues. Incoming bootstrap events enter service and
 * depart after an inter-arrival time sampled from the class-specific
 * distribution. On departure, source::dequeue() clones the departed event into
 * a recycled event object and immediately schedules the next arrival at the
 * source, keeping the external arrival stream alive.
 *
 * Like des::station, the source is templated on the distribution: T<TT> is a
 * <random> distribution, e.g. source<double, exponential_distribution> for a
 * Poisson arrival stream.
 */
template <typename TT, template <typename> typename T> class des::source : public des::sourcesink
{
	public:
		/**
		 * @brief Construct a source with class-specific inter-arrival distributions.
		 *
		 * Each class samples its inter-arrival times from its own random stream
		 * (see node::set_streams()).
		 *
		 * @param rand_dist Inter-arrival time distributions, indexed by event class.
		 * @param description String identifier.
		 */
		source(vector<shared_ptr<T<TT>>> rand_dist, string description);
		/**
		 * @brief Construct a one-class source without configuring its distribution.
		 *
		 * This constructor is useful for tests and custom subclasses. Calling
		 * get_service() requires a configured distribution, so the
		 * distribution-based constructor is the usual simulation entry point.
		 *
		 * @param description String identifier.
		 */
		source(string description);
		/**
		 * @brief Select the source server that will schedule the next arrival.
		 *
		 * Sources do not use a waiting queue; all admitted events use server 0.
		 *
		 * @return Always 0.
		 */
		int schedule(const shared_ptr<event>& e, const vector<vector<int>>& s_map) override;
		/**
		 * @brief Select a waiting queue for an event rejected from service.
		 *
		 * Sources do not enqueue waiting jobs. The method is present to satisfy
		 * the node interface and returns 0 when called.
		 *
		 * @return Always 0.
		 */
		int enqueue(const shared_ptr<event>& e, const vector<vector<int>>& q_map) override;
		/**
		 * @brief Schedule the next external arrival after a source departure.
		 *
		 * Reuses an event from the shared source/sink pool, clones the departed
		 * event metadata into it, and re-arrives it at the source.
		 *
		 * @return Always 0.
		 */
		int dequeue(const shared_ptr<event>& e, const vector<vector<int>>& s_map) override;
		// Utility methods
		/**
		 * @brief Reset the source and optionally snapshot attached observers.
		 *
		 * Delegates to sourcesink::reset(), preserving @p newrun so observers
		 * attached to the source can store completed-run values.
		 *
		 * @param value Elapsed simulation time to subtract from pending events.
		 * @param keys Additional event-info keys whose times should be shifted.
		 * @param newrun When true, observers snapshot their current-run values.
		 */
		inline void reset(double value, vector<tag> keys = vector<tag>(), bool newrun = false) override
		{
			this -> sourcesink::reset(value, keys, newrun);
		}
		/**
		 * @brief Clear source state and attached observer state.
		 */
		inline void clear() override
		{
			this -> sourcesink::clear();
		}
		/**
		 * @brief Return the inter-arrival distribution of one class.
		 *
		 * @param cls Event-class index.
		 * @return The distribution, shared with the source: changing its
		 *         parameters changes the arrivals the source generates.
		 */
		inline shared_ptr<T<TT>> get_rng(unsigned int cls) const
		{
			return rng.at(cls);
		}
		/**
		 * @brief Replace the inter-arrival distribution of one class.
		 *
		 * @param dist New inter-arrival distribution.
		 * @param cls Event-class index.
		 */
		inline void set_rng(shared_ptr<T<TT>> dist, unsigned int cls)
		{
			rng.at(cls) = dist;
		}
		/**
		 * @brief Return a human-readable source summary.
		 *
		 * @return String containing the source prefix and base node summary.
		 */
		string to_string() const override;
	protected:
		/**
		 * @brief Sample the next inter-arrival time for a class.
		 *
		 * @param cls Event-class index.
		 * @param idx Server index selected by schedule(); ignored by source.
		 * @return Sampled inter-arrival time.
		 */
		double get_service(unsigned int& cls, unsigned int& idx) override;
	private:
		vector<shared_ptr<T<TT>>> rng; ///< Per-class inter-arrival distributions.
};
#endif
