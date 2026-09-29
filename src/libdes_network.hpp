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
					int (*hffunc)(const shared_ptr<event>&, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
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
					pair<bool, int> (*hbfunc)(const shared_ptr<event>&, int,
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
					int (*hffunc)(const shared_ptr<event>&, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
					pair<bool, int> (*hbfunc)(const shared_ptr<event>&, int,
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
			 */
	        void route(const shared_ptr<event>& e);
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
			 * @brief Return the current-run throughput estimate for an edge and class.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Events routed on the edge in the current run divided by the current
			 *         run time (time of the last event returned by next_event()), or zero
			 *         before any event has been processed.
			 */
			inline double flow(int source, int destination, int cls)
			{
				return now > 0 ? get_count(source, destination, cls) / now : 0.0;
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
				return edge_observer<counter>(SIGNAL_NET_ROUTING, source, destination).get(cls);
			}
			/**
			 * @brief Return the cross-run mean flow for an edge and class.
			 *
			 * Each completed run contributes its throughput N/T (events routed on the
			 * edge divided by the run length), recorded by reset().
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Mean flow over completed runs, or zero if no runs are stored.
			 */
			inline double get_flow(int source, int destination, int cls)
			{
				return edge_observer<scalar>(SIGNAL_NET_FLOW, source, destination).get_scalar(cls);
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
			if(it == fnd->second.end())
				throw std::out_of_range("Observer index " + std::to_string(idx) + " out of range");
			return *it;
		}
			/**
			 * @brief Return the cross-run standard deviation of edge flow.
			 *
			 * @param source Source node index.
			 * @param destination Destination node index.
			 * @param cls Event class index.
			 * @return Standard deviation of completed-run flow values.
			 */
			inline double get_flow_stddev(int source, int destination, int cls)
			{
				return edge_observer<scalar>(SIGNAL_NET_FLOW, source, destination).stddev(cls);
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
				return edge_observer<counter>(SIGNAL_NET_ROUTING, source, destination).confidence_interval(alpha);
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
				return edge_observer<scalar>(SIGNAL_NET_FLOW, source, destination).confidence_interval(alpha);
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
				return edge_observer<counter>(SIGNAL_NET_ROUTING, source, destination).confidence_interval(alpha, cls);
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
				return edge_observer<scalar>(SIGNAL_NET_FLOW, source, destination).confidence_interval(alpha, cls);
			}
			/**
			 * @brief Return how many times, in the current run, the destination of an
			 *        edge refused an event of class @p cls.
			 *
			 * Refusals of rerouting attempts are counted on the rerouting edge.
			 */
			inline int get_blocked(int source, int destination, int cls)
			{
				return edge_observer<counter>(SIGNAL_NET_BLOCK, source, destination).get(cls);
			}
			/**
			 * @brief Return how many events of class @p cls departing node @p source were
			 *        lost in the current run because no destination accepted them.
			 */
			inline int get_lost(int source, int cls)
			{
				return front_observer<counter>(SIGNAL_NET_LOSS + "_" + std::to_string(source)).get(cls);
			}
			/**
			 * @brief Set how many times the block handler may reroute one event before it is lost.
			 *
			 * The default is the number of nodes in the network.
			 */
			inline void set_max_reroute_attempts(unsigned int attempts)
			{
				max_reroute_attempts = attempts;
			}
			/**
			 * @brief Serialize the network routing table to a human-readable string.
			 *
			 * @return Text representation of the routing tensor.
			 */
			string to_string() const;
			/**
			 * @brief Close the current run and reset all network observers and node state.
			 *
			 * Before observers are reset, every edge records its run throughput N/T,
			 * with N the events routed on the edge and T = @p time.
			 *
			 * @param time Length of the run being closed; it is also subtracted from scheduled event times.
			 * @param keys Additional event-info keys whose stored times should be shifted.
			 * @param newrun When true, observers snapshot the current run before clearing.
			 */
			void reset(double time, vector<tag> keys = vector<tag>(), bool newrun = false);
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
				for(const shared_ptr<observer>& obs: it -> second)
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
			// void handle_constraints(const shared_ptr<event>& e, const double& time);
			/**
			 * @brief Insert an event into the node identified by string id.
			 *
			 * @param e Event to insert.
			 * @param node Destination node string identifier.
			 * @return true when the node is found and accepts the event.
			 */
			bool insert(const shared_ptr<event>& e, string node);

			/**
			 * @brief Default blocking handler.
			 *
			 * @return Pair whose first value tells whether rerouting should be attempted
			 *         and whose second value is the reroute destination.
			 */
			virtual pair<bool, int> hbfunc(const shared_ptr<event>&, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g);
			/**
			 * @brief Default routing handler.
			 *
			 * Samples the routing tensor for the event's current node and class.
			 *
			 * @return Destination node index, or -1 when the event leaves the network.
			 */
			virtual int hffunc(const shared_ptr<event>&, const vector<vector<vector<double>>> &, shared_ptr<mt19937_64>& g);
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
			pair<bool, int> (*handle_block)(const shared_ptr<event>&, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g) = nullptr;
			/**
			 * @brief Optional function used to choose the next destination for an event.
			 */
			int (*handle_forks)(const shared_ptr<event>&, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&) = nullptr;
			/**
			 * @brief Current simulation time: time of the last event returned by next_event().
			 */
			double now = 0.0;
			/**
			 * @brief Maximum number of reroutes attempted for one blocked event before it is lost.
			 */
			unsigned int max_reroute_attempts = 0;
			/**
			 * @brief Binary min-heap of the node indexes with a pending departure, ordered by
			 *        {node_heap_time, node_index} (ties go to the lowest node index).
			 *
			 * Entries are present only for nodes with a finite pending event time.
			 */
			vector<int> event_heap;
			/**
			 * @brief Position of each node in @c event_heap, or -1 if it has no pending event.
			 */
			vector<int> heap_pos;
			/**
			 * @brief Heap time currently registered for each node.
			 *
			 * A node with no pending event stores @c __DBL_MAX__.
			 */
			vector<double> node_heap_time;
			/**
			 * @brief Resynchronize @c event_heap after a node's next-event time changes.
			 *
			 * @param idx Node index to update.
			 */
			void update_heap(int idx);
			/**
			 * @brief Heap order: earlier time first, lower node index on ties.
			 */
			inline bool heap_less(int a, int b) const
			{
				return node_heap_time[a] < node_heap_time[b] || (node_heap_time[a] == node_heap_time[b] && a < b);
			}
			/**
			 * @brief Swap two heap positions, keeping @c heap_pos consistent.
			 */
			inline void heap_swap(size_t i, size_t j)
			{
				std::swap(event_heap[i], event_heap[j]);
				heap_pos[event_heap[i]] = static_cast<int>(i);
				heap_pos[event_heap[j]] = static_cast<int>(j);
			}
			/**
			 * @brief Move the entry at heap position @p i up/down to restore the heap order.
			 */
			void heap_sift_up(size_t i);
			void heap_sift_down(size_t i);
			/**
			 * @brief Observer lists of one edge's signals, resolved once (nullptr if the signal does not exist).
			 */
			struct edge_signals
			{
				list<shared_ptr<observer>>* route = nullptr;
				list<shared_ptr<observer>>* block = nullptr;
			};
			/**
			 * @brief Observer lists of @c SIGNAL_NET_ROUTING and @c SIGNAL_NET_BLOCK, per edge [source][destination].
			 */
			vector<vector<edge_signals>> edge_lists;
			/**
			 * @brief Observer lists of @c SIGNAL_NET_LOSS, per node.
			 */
			vector<list<shared_ptr<observer>>*> loss_lists;
			/**
			 * @brief The object the observer lists above point into; a copied network re-resolves them.
			 */
			const network* lists_owner = nullptr;
			/**
			 * @brief Resolve @c edge_lists and @c loss_lists into this object's @c observable_events.
			 */
			void index_signals();
			/**
			 * @brief Deliver @p msg to every observer in @p obs, if any.
			 */
			static inline void deliver(list<shared_ptr<observer>>* obs, message& msg)
			{
				if(obs != nullptr)
				{
					for(const shared_ptr<observer>& o: *obs)
					{
						o -> update(msg);
					}
				}
			}
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
			/**
			 * @brief Notify the observers of edge @p source -> @p dest about a routed event.
			 *
			 * Does nothing when no observer is attached to the edge.
			 */
			void notify_routing(const shared_ptr<event>& e, int source, int dest);
			/**
			 * @brief Notify the observers in @p obs with a message carrying only the event class.
			 */
			void notify_class(list<shared_ptr<observer>>* obs, int cls);
			/**
			 * @brief Deliver @p e to node @p dest, counting the routing or the refusal on edge @p source -> @p dest.
			 *
			 * @return true when @p dest accepted the event.
			 */
			bool try_arrival(const shared_ptr<event>& e, int source, int dest);
			/**
			 * @brief Return the first observer attached to @p signal if it has type O, nullptr otherwise.
			 */
			template <typename O>
			O* find_front(const string& signal)
			{
				auto fnd = observable_events.find(signal);
				if(fnd == observable_events.end() || fnd->second.empty())
				{
					return nullptr;
				}
				return dynamic_cast<O*>(fnd->second.front().get());
			}
			/**
			 * @brief Return the first observer attached to @p signal.
			 *
			 * @throws invalid_argument if the signal is not defined or its first observer is not an O.
			 */
			template <typename O>
			O& front_observer(const string& signal)
			{
				if(observable_events.find(signal) == observable_events.end())
				{
					throw invalid_argument("Measurable event " + signal + " is not defined in network " + get_sid());
				}
				O* obs = find_front<O>(signal);
				if(obs == nullptr)
				{
					throw invalid_argument("No observer of the expected type is attached to " + signal + " in network " + get_sid());
				}
				return *obs;
			}
			/**
			 * @brief Return the first observer attached to signal @p prefix_<source>_<destination>.
			 */
			template <typename O>
			O& edge_observer(const string& prefix, int source, int destination)
			{
				return front_observer<O>(prefix + edge(source, destination));
			}
			/**
			 * @brief Return the signal suffix identifying edge @p source -> @p destination.
			 */
			static inline string edge(int source, int destination)
			{
				return "_" + std::to_string(source) + "_" + std::to_string(destination);
			}
	};

#endif
