#ifndef NETWORK_H
#define NETWORK_H

#include "libdes_node.hpp"
#include "libdes_event.hpp"
#include "libdes_scalar.hpp"
#include "libdes_counter.hpp"
// #include "../Constraint/Constraint.hpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <list>
#include <set>
#include <vector>
#include <random>
#include <memory>
#include <unordered_map>

using namespace std;

namespace des
{
	class network;
}

	/**
	 * @brief Discrete-event network coordinator.
	 *
	 * A network owns the ordered list of nodes participating in a simulation and
	 * the routing tensor used to move departing events to their next destination.
	 * It also keeps network-level observers for per-edge counts and flow estimates.
	 */
	class des::network : public des::observable
	{
		public:
			/**
			 * @brief Construct an empty network.
			 */
			network(){}
			/**
			 * @brief Construct a network using the default routing and blocking handlers.
			 *
			 * @param nds Nodes indexed by position; routing destinations refer to these indices.
			 * @param rtg Routing probabilities as rtg[source][destination][class].
			 * @param g Shared pseudo-random generator used by routing decisions.
			 */
			network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, shared_ptr<mt19937_64>& g);
			/**
			 * @brief Construct a network with a custom routing/fork handler.
			 *
			 * @param nds Nodes indexed by position; routing destinations refer to these indices.
			 * @param rtg Routing probabilities as rtg[source][destination][class].
			 * @param hffunc Function used to choose the next destination for a departed event.
			 * @param g Shared pseudo-random generator used by routing decisions.
			 */
			network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
					int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
					shared_ptr<mt19937_64>& g);
			/**
			 * @brief Construct a network with a custom blocking handler.
			 *
			 * @param nds Nodes indexed by position; routing destinations refer to these indices.
			 * @param rtg Routing probabilities as rtg[source][destination][class].
			 * @param hbfunc Function used when a destination node rejects an event.
			 * @param g Shared pseudo-random generator used by routing decisions.
			 */
			network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
					pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
											const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
					shared_ptr<mt19937_64>& g);
			/**
			 * @brief Construct a network with custom routing and blocking handlers.
			 *
			 * @param nds Nodes indexed by position; routing destinations refer to these indices.
			 * @param rtg Routing probabilities as rtg[source][destination][class].
			 * @param hffunc Function used to choose the next destination for a departed event.
			 * @param hbfunc Function used when a destination node rejects an event.
			 * @param g Shared pseudo-random generator used by routing decisions.
			 */
			network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
					int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
					pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
											const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
					shared_ptr<mt19937_64>& g);
			// Constraint-handler constructors are kept here as design notes for a
			// future extension, but the Constraint type is not part of this build.
			// network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>>  const_handler, shared_ptr<mt19937_64>& g);
			// network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, vector<vector<vector<double>>> rtg_fail, vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>>  const_handler, shared_ptr<mt19937_64>& g);
			/**
			 * @brief Route a departed event to its next node.
			 *
			 * The source node is read from @c EVENT_NODE on @p e. The selected
			 * destination is produced by the custom routing handler, when present, or
			 * by the default routing-table sampler. Successful arrivals update the
			 * destination node's heap entry and notify network routing observers.
			 *
			 * @param e Event that just departed from its current node.
			 * @param time Current simulation time.
			 */
	        void route(shared_ptr<event> e, const double& time);
			/**
			 * @brief Dequeue and return the next event scheduled anywhere in the network.
			 *
			 * The heap is synchronized lazily if it is empty, which supports callers that
			 * inject the first event directly into a node after constructing the network.
			 *
			 * @return Next event to route, or @c nullptr when no pending event exists.
			 **/
			shared_ptr<event> next_event();
			/**
			 * @brief Return the current-run flow observer value for an edge and class.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Current-run scalar value stored by the edge flow observer.
			 */
			inline double flow(int source, int destination, int cls)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				++(lst); //The second element of the list is the observer for the flow
				return dynamic_cast<scalar*>((*lst).get())-> get(cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return the current-run event count for an edge and class.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Number of events routed on the edge in the current run.
			 */
			inline int get_count(int source, int destination, int cls)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				//The second element of the list is the observer for the flow
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				return dynamic_cast<counter*>((*lst).get()) -> get(cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return the cross-run mean flow for an edge and class.
			 *
			 * This reads the completed-run means stored by the edge flow observer.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Mean flow over completed runs, or zero if no runs are stored.
			 */
			inline double get_flow(int source, int destination, int cls)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				++(lst); //The second element of the list is the observer for the flow
				return dynamic_cast<scalar*>((*lst).get())-> get_scalar(cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return the observer at a signal-local index.
			 *
			 * @param signal Signal identifier, for example @c SIGNAL_NET_ROUTING + "_0_1".
			 * @param idx Zero-based position in the signal's observer list.
			 * @return Shared pointer to the requested observer.
			 * @throws invalid_argument if @p signal is not registered.
			 * @throws out_of_range if @p idx is not present for the signal.
			 */
			inline shared_ptr<observer> get_observer(const string& signal, unsigned int idx)
		{
			auto fnd = observable_events.find(signal);
			if(fnd == observable_events.end())
				throw invalid_argument("Signal " + signal + " not found in network " + get_sid());
			auto it = fnd->second.begin();
			for(unsigned int i = 0; i < idx; ++i)
			{
				if(++it == fnd->second.end())
					throw std::out_of_range("Observer index " + std::to_string(idx) + " out of range");
			}
			return *it;
		}
			/**
			 * @brief Return the cross-run standard deviation of edge flow.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Standard deviation of completed-run flow means.
			 */
			inline double get_flow_stddev(int source, int destination, int cls)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				++(lst); //The second element of the list is the observer for the flow
				return dynamic_cast<scalar*>((*lst).get())-> stddev(cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return count confidence intervals for every class on an edge.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param alpha Significance level used by the observer CI calculation.
			 * @return One confidence interval per event class.
			 */
			inline vector<pair<double,double>> get_count_ci(int source, int destination, double alpha)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				//The second element of the list is the observer for the flow
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				return dynamic_cast<counter*>((*lst).get()) -> confidence_interval(alpha);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return flow confidence intervals for every class on an edge.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param alpha Significance level used by the observer CI calculation.
			 * @return One confidence interval per event class.
			 */
			inline vector<pair<double,double>> get_flow_ci(int source, int destination, double alpha)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				++(lst); //The second element of the list is the observer for the flow
				return dynamic_cast<scalar*>((*lst).get()) -> confidence_interval(alpha);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return the count confidence interval for one edge and class.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @param alpha Significance level used by the observer CI calculation.
			 * @return Confidence interval for the completed-run counts.
			 */
			inline pair<double,double> get_count_ci(int source, int destination, int cls, double alpha)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				//The second element of the list is the observer for the flow
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				return dynamic_cast<counter*>((*lst).get()) -> confidence_interval(alpha, cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Return the flow confidence interval for one edge and class.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @param alpha Significance level used by the observer CI calculation.
			 * @return Confidence interval for the completed-run flow estimates.
			 */
			inline pair<double,double> get_flow_ci(int source, int destination, int cls, double alpha)
		{
			string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
			std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
			if(fnd != observable_events.end())
			{
				std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
				++(lst); //The second element of the list is the observer for the flow
				return dynamic_cast<scalar*>((*lst).get()) -> confidence_interval(alpha, cls);
			}
			else
			{
				throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
			}
		}
			/**
			 * @brief Serialize the network routing table to a human-readable string.
			 *
			 * @return Text representation of the routing tensor.
			 */
			string to_string() const;
			/**
			 * @brief Reset all network observers and node state.
			 *
			 * @param time Amount of elapsed time to subtract from scheduled event times.
			 * @param keys Additional event-info keys whose stored times should be shifted.
			 * @param newrun When true, observers snapshot the current run before clearing.
			 */
			void reset(double time, vector<string> keys = vector<string>(), bool newrun = false);
			/**
			 * @brief Notify all observers attached to a network-level signal.
			 *
			 * @param signal Signal identifier.
			 * @param msg Message payload delivered to every attached observer.
			 */
			inline void notify(string signal, message& msg) override
		{
			unordered_map<string, list<shared_ptr<observer>>>::iterator it = observable_events.find(signal);
			if(it != observable_events.end())
			{
				for(shared_ptr<observer> obs: it -> second)
				{
					obs.get()->update(msg);
				}
			}
		}
		protected:
			/**
			 * @brief Nodes that participate in the network.
			 *
			 * The vector index is the node identifier used by the routing tensor.
			 */
	        vector<shared_ptr<node>> nodes;
			/**
			 * @brief Routing probabilities indexed as routing[source][destination][class].
			 *
			 * The default routing handler treats each source row as a cumulative
			 * probability input: for a departing event of class k from node i,
			 * routing[i][j][k] is the probability mass assigned to destination j.
			 */
	        vector<vector<vector<double>>> routing;
			// Placeholder for per-node, per-class constraint handlers.
			// vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>> handler;
			// Placeholder for applying time-dependent and state-dependent constraints.
			// void handle_constraints(shared_ptr<event> e, const double& time);
			/**
			 * @brief Insert an event into the node identified by string id.
			 *
			 * @param e Event to insert.
			 * @param node Destination node string identifier.
			 * @return true when the node is found and accepts the event.
			 */
			bool insert(shared_ptr<event> e, string node);

			/**
			 * @brief Default blocking handler.
			 *
			 * @return Pair whose first value tells whether rerouting should be attempted
			 *         and whose second value is the reroute destination.
			 */
			virtual pair<bool, int> hbfunc(shared_ptr<event>, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g);
			/**
			 * @brief Default routing handler.
			 *
			 * Samples the routing tensor for the event's current node and class.
			 *
			 * @return Destination node index, or -1 when the event leaves the network.
			 */
			virtual int hffunc(shared_ptr<event>, const vector<vector<vector<double>>> &, shared_ptr<mt19937_64>& g);
		private:
			/**
			 * @brief Shared simulator-wide pseudo-random generator.
			 */
	        shared_ptr<mt19937_64> gen;
			// Placeholder for rejected or constrained movement counts.
	        // vector<vector<vector<int>>> count_constrained_events;
			/**
			 * @brief Optional function used when a destination node rejects an event.
			 */
			pair<bool, int> (*handle_block)(shared_ptr<event>, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g);
			/**
			 * @brief Optional function used to choose the next destination for an event.
			 */
			int (*handle_forks)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&);
			/**
			 * @brief Min-heap index of pending departures as {next_event_time, node_index}.
			 *
			 * Entries are present only for nodes with a finite pending event time.
			 */
			set<pair<double, int>> event_heap;
			/**
			 * @brief Heap time currently registered for each node.
			 *
			 * A node with no pending event stores @c __DBL_MAX__. The value is used
			 * to erase stale heap entries before re-inserting an updated one.
			 */
			vector<double> node_heap_time;
			/**
			 * @brief Resynchronize @c event_heap after a node's next-event time changes.
			 *
			 * @param idx Node index to update.
			 */
			void update_heap(int idx);
			/**
			 * @brief Return true when a next-event time represents a scheduled event.
			 */
			static inline bool has_pending_event(double time)
			{
				return time < __DBL_MAX__;
			}
			/**
			 * @brief Rebuild @c event_heap and @c node_heap_time from current node state.
			 */
			void init_heap();
	};

#endif
