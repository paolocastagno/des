#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>
#include <random>

#include "libdes_object.hpp"
#include "libdes_observable.hpp"
#include "libdes_observer.hpp"
#include "libdes_message.hpp"
#include "libdes_event.hpp"
#include "libdes_queue.hpp"
#include "libdes_policy.hpp"
#include "libdes_const.hpp"
#include "libdes_random.hpp"

using namespace std;

namespace des
{
	class node;
}

/**
 * @brief Class node
 * 
 */
class des::node : public des::object, public des::observable
{
	public:
		typedef int (*service_pick_handler)(const shared_ptr<event>&, int, const vector<vector<int>>&, const vector<shared_ptr<queue>>&, random_engine&);
		// Constructor & destructor
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 */
		node();
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param descriptioin Description of the node
		 * 
		 */
		node(string description);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param descriptioin Description of the node
		 * @param cls number of classes handled
		 */
		node(string description, int cls);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param cls number of classes handled  
		 * @param descriptioin Description of the node
		 */
		node(unsigned int cls, vector<shared_ptr<queue>> q_vec, vector<shared_ptr<queue>> s_vec, vector<vector<int>> qmap, vector<vector<int>> smap, string description);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param cls number of classes handled  
		 * @param descriptioin Description of the node
		 */
		node(unsigned int cls, string description);
		/**
		 * @brief Set the custom handler used to pick the next waiting queue when moving jobs to service.
		 * 
		 * @param handler function pointer. If nullptr, default dequeue() logic is used.
		 */
		inline void set_service_pick_handler(service_pick_handler handler)
		{
			handle_service_pick = handler;
		}
		/**
		 * @brief Give the node its own block of random streams.
		 *
		 * Stream 0 draws the node's choices among queues and servers, stream 1 + c the service
		 * times of class c. The network gives each of its nodes a distinct block when it is
		 * constructed (see network::network); call this afterwards to choose another block.
		 * Until then, every node draws from the block of a default-seeded engine.
		 *
		 * @param block engine positioned at the start of the block
		 */
		inline void set_streams(const random_engine& block)
		{
			streams = stream_block(block);
		}
		/**
		 * @brief Destroy the node object
		 * 
		 */
		virtual ~node();
		// Get & set methods
		/**
		 * @brief Update all the counters concerning the departure of event *e* at time *time* 
		 * `
		 * @param q_vec vector of pointers to the queues
		 * 
		 */	
		inline void set_queue(vector<shared_ptr<queue>>& q_vec)
		{
			q = q_vec;
		}
		/**
		 * @brief Update all the counters concerning the departure of event *e* at time *time* 
		 * `
		 * @param s_vec vector of pointers to the queues
		 * 
		 */	
		inline void set_server(vector<shared_ptr<queue>>& s_vec)
		{
			s = s_vec;
			refresh_next();
		}
		/**
		 * @brief Set queue-to-class mapping matrix.
		 * q_map[q][cls] != 0 means class cls can use queue q.
		 */
		inline void set_queue_map(const vector<vector<int>>& map)
		{
			q_map = map;
		}
		/**
		 * @brief Set server-to-class mapping matrix.
		 * s_map[srv][cls] != 0 means class cls can use server srv.
		 */
		inline void set_server_map(const vector<vector<int>>& map)
		{
			s_map = map;
		}
		/**
		 * @brief Returns the number of events of the given class currently in queue
		 * 
		 * @return int 
		 */
		inline int queue_length(int cls)
		{
			return q.at(cls) -> in_queue(); 
		}
		/**
		 * @brief Returns the number of events of the given class currently in queue
		 * 
		 * @return int 
		 */
		inline int queue_length()
		{
			int len = 0;
			for(unsigned int i = 0; i < q.size(); i++)
				len += q.at(i) -> in_queue();
			return len;
		}
		/**
		 * @brief Returns the number of events of the given class currently in service
		 * 
		 * @return int 
		 */
		inline int service_length(unsigned int cls)
		{
			return s.at(cls) -> in_queue(); 
		}
		/**
		 * @brief Returns the number of events of the currently in service
		 * 
		 * @return int 
		 */
		inline int service_length()
		{
			int len = 0;
			for(unsigned int i = 0; i < s.size(); i++)
				len += s.at(i) -> in_queue(); 
			return len;
		}
		// Manage the events in the node
		/**
		 * @brief Handle a new arrival of the event *e* at time *time*.
		 * 
		 * Subclasses may override it to handle arrivals differently (e.g. des::sink absorbs them).
		 *
		 * @param time time
		 * @param e *const* pointer to the arriving
		 * @return true if the event is allowed to get in the node, false otherwise
		 */
		virtual bool arrival(const shared_ptr<event>& e);
		/**
		 * @brief Handle a departure of the event *e* at time *time*.
		 * 
		 * @param time time
		 * @return Pointer to the departing event
		 */
		shared_ptr<event> departure();
		/**
		 * @brief Get the time of the next departure, __DBL_MAX__ if no job is in service
		 *
		 * @return double
		 */
		virtual double next_event_time() const;
		// Utility methods
		/**
		 * @brief resets the state of the node
		 * 
		 * @param time 
		 */
		void reset(double time, vector<tag> keys = vector<tag>(), bool newrun = false) override;
		/**
		 * @brief Restore the node to the initial state
		 * 
		 */
		void clear() override;
		/**
		 * @brief Writes the relevant information about the measure to string
		 * 
		 * @return string 
		 */
		virtual string to_string() const override;
		/**
		 * @brief notify a message to an observer
		 * 
		 * @param signal 
		 * @param msg 
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
		 * @brief Record the arrival of @p e and its departure at the same time, as a server with
		 *        zero service time would, without holding the job.
		 *
		 * Updates the counters, sets and clears the tags as arrival() and departure() do (with
		 * zero sojourn, wait and service), and notifies the arrival, service and departure signals.
		 * No departure event is scheduled.
		 */
		void pass_through(const shared_ptr<event>& e);
		/**
		 * @brief Maps classes to queues. q_map[i,j] tells wheter the i-th class is allowed to use the j-th queue
		 * 
		 */
		vector<vector<int>> q_map;
		/**
		 * @brief Maps classes to servers. s_map[i,j] tells wheter the i-th class is allowed to use the j-th server
		 * 
		 */
		vector<vector<int>> s_map;
		/**
		 * @brief node's queue(s)
		 * 
		 */
		vector<shared_ptr<queue>> q;
		/**
		 * @brief node's server(s)
		 * 
		 */
		vector<shared_ptr<queue>> s;
		/**
		 * @brief Random stream of the node's choices among queues and servers
		 */
		inline random_engine& choice_stream()
		{
			return streams.at(0);
		}
		/**
		 * @brief Random stream of the service times of class @p cls
		 */
		inline random_engine& service_stream(unsigned int cls)
		{
			return streams.at(1 + static_cast<size_t>(cls));
		}
		/**
		 * @brief Get the average service time for class cls
		 * 
		 * @return double 
		 */
		virtual double get_service(unsigned int& cls, unsigned int& idx) = 0;
		/**
		 * @brief Handle a transition of a job from  the queue to the service  
		 * 
		 */
		virtual int schedule(const shared_ptr<event>& e, const vector<vector<int>>& s_map) = 0;
		/**
		 * @brief Chooses one among the available queue for the current job
		 * 
		 * @return an integer used to index the right queue in the q_map structure.
		 * 
		 */
		virtual int enqueue(const shared_ptr<event>& e, const vector<vector<int>>& q_map) = 0;
		/**
		 * @brief Chooses from which queue to pick the next job among the available queues
		 * 
		 * @return an integer used to index the right queue in the s_map structure.
		 */
		virtual int dequeue(const shared_ptr<event>& e, const vector<vector<int>>& s_map) = 0;
		/**
		 * @brief Default service selection handler. Returns the queue index selected by dequeue().
		 */
		virtual int shfunc(const shared_ptr<event>& e, int sched, const vector<vector<int>>& qmap, const vector<shared_ptr<queue>>& queues, random_engine& g);
		/**
		 * @brief Tells whether server @p srv may serve jobs of class @p cls according to s_map.
		 * Classes or servers missing from s_map are not restricted.
		 */
		inline bool can_serve(unsigned int srv, unsigned int cls) const
		{
			if(srv >= s_map.size() || cls >= s_map.at(srv).size())
			{
				return true;
			}
			return s_map.at(srv).at(cls) != 0;
		}
	private:
		// ids generator
		inline static atomic<unsigned int> id_gen{0};
		/**
		 * @brief The node's random streams, see set_streams()
		 */
		stream_block streams;
		// Measures
        vector<long> in;
        vector<long> out;
        vector<double> usage;
        vector<double> last_event;
		/**
		 * @brief Optional custom callback used to select the queue index for the next job entering service.
		 */
		service_pick_handler handle_service_pick;
		/**
		 * @brief The node's signals, indexing @c signal_lists
		 */
		enum node_signal { SIG_ARRIVAL = 0, SIG_SERVICE = 1, SIG_DEPARTURE = 2 };
		/**
		 * @brief Observer lists of SIGNAL_NODE_ARRIVAL, SIGNAL_NODE_SERVICE and SIGNAL_NODE_DEPARTURE,
		 *        resolved once (nullptr if the signal does not exist)
		 */
		list<shared_ptr<observer>>* signal_lists[3] = {nullptr, nullptr, nullptr};
		/**
		 * @brief The object @c signal_lists point into; a copied node re-resolves them
		 */
		const node* lists_owner = nullptr;
		/**
		 * @brief Notify the observers of signal @p which with a view of @p e's fields
		 */
		void notify_signal(node_signal which, const shared_ptr<event>& e);
		// Utility methods
		/**
		 * @brief Updates counters for a new arrival
		 * 
		 * @param cls is the class identifier of the arriving event
		 * @param time the arrival time 
		 * 
		 */
		void update_in(unsigned int& cls, double& time);
		/**
		 * @brief Updates counters for a new departure
		 * 
		 * @param cls is the class identifier of the arriving event
		 * @param time the depaprture time 
		 * 
		 */
		void update_out(unsigned int& cls, double& time);
		/**
		 * @brief Returns the index of the server finishing the job
		 * 
		 * @return an integer used to index the right queue in the s_map structure.
		 */
		int idx_next_departure() const;
		/**
		 * @brief Time and server of the next departure, kept up to date by refresh_next()
		 */
		double next_time = __DBL_MAX__;
		int next_server = 0;
		/**
		 * @brief Recompute next_time and next_server from the servers.
		 *
		 * The servers change only through arrival(), departure(), reset(), clear() and
		 * set_server(), which call it after the change.
		 */
		void refresh_next();
};
#endif
