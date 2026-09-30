#ifndef SINK_H
#define SINK_H

#include "libdes_sourcesink.hpp"

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
	class sink;
}

/**
 * @brief Terminal node that consumes routed events.
 *
 * A sink absorbs every arrival at once: it records the arrival and the departure
 * of the job at the same time (its observers see a zero sojourn), then disposes the
 * event into the shared source/sink event pool. It never schedules a departure
 * event, so network::next_event() returns no event from a sink. It is the usual
 * terminal node for open networks.
 */
class des::sink : public des::sourcesink
{
	public:
		/**
		 * @brief Construct a sink with placeholder rate metadata.
		 *
		 * This overload is declared for API compatibility. The current sink
		 * implementation does not use service rates or random sampling.
		 *
		 * @param r Unused placeholder service-rate vector.
		 * @param description String identifier.
		 */
		sink(vector<double> r, string description);
		/**
		 * @brief Construct a sink for a fixed number of event classes.
		 *
		 * @param description String identifier.
		 * @param cls Number of event classes.
		 */
		sink(string description, int cls);
		/**
		 * @brief Construct a one-class sink.
		 *
		 * @param description String identifier.
		 */
		sink(string description);
		/**
		 * @brief Destroy the sink.
		 */
		~sink();
		/**
		 * @brief Absorb the arriving event: record its arrival and departure, then dispose it.
		 *
		 * @return Always true.
		 */
		bool arrival(const shared_ptr<event>& e) override;
		/**
		 * @brief Select the sink server for an arriving event.
		 *
		 * Sinks accept all classes through server 0.
		 *
		 * @return Always 0.
		 */
		int schedule(const shared_ptr<event>& e, const vector<vector<int>>& s_map) override;
		/**
		 * @brief Select a waiting queue for a sink arrival.
		 *
		 * Sink arrivals are terminal and do not wait in queues. The method is
		 * present to satisfy the node interface and returns 0 when called.
		 *
		 * @return Always 0.
		 */
		int enqueue(const shared_ptr<event>& e, const vector<vector<int>>& q_map) override;
		/**
		 * @brief Dispose the completed event into the shared event pool.
		 *
		 * arrival() absorbs the jobs without holding them, so a sink never
		 * reaches this method itself; it is kept to satisfy the node interface.
		 *
		 * @return Always 0.
		 */
		int dequeue(const shared_ptr<event>& e, const vector<vector<int>>& s_map) override;
		// Utility methods
		/**
		 * @brief Reset the sink and optionally snapshot attached observers.
		 *
		 * Delegates to sourcesink::reset(), preserving @p newrun so sink
		 * observers can store completed-run values.
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
		 * @brief Clear sink state and attached observer state.
		 */
		inline void clear() override
		{
			this -> sourcesink::clear();
		}
		/**
		 * @brief Return a human-readable sink summary.
		 *
		 * @return String containing the sink prefix and base node summary.
		 */
		string to_string() const override;
	protected:
		/**
		 * @brief Return the sink service time.
		 *
		 * Sink service is instantaneous, so this always returns 0.
		 *
		 * @param cls Event-class index; ignored.
		 * @param idx Server index; ignored.
		 * @return Always 0.
		 */
		inline double get_service(unsigned int&, unsigned int&) override
		{
			return 0;
		}
	private:
        vector<double> rate; ///< Placeholder rate storage for compatibility.
        vector<exponential_distribution<double>> exp; ///< Placeholder distributions for compatibility.
};
#endif
