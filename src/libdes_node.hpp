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
		 * @param g The global mersenne twister random number generator used to generate Events' service time
		 */
		node(string description, shared_ptr<mt19937_64> g);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param descriptioin Description of the node
		 * @param g The global mersenne twister random number generator used to generate Events' service time
		 */
		node(string description, int cls);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param descriptioin Description of the node
		 * @param g The global mersenne twister random number generator used to generate Events' service time
		 */
		node(string description, int cls, shared_ptr<mt19937_64> g);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param cls number of classes handled  
		 * @param descriptioin Description of the node
		 * @param g The global mersenne twister random number generator used to generate events' service time
		 */
		node(unsigned int cls, vector<shared_ptr<queue>> q, vector<shared_ptr<queue>>, vector<vector<int>> qmap, vector<vector<int>> smap, string description, shared_ptr<mt19937_64> g);
		/**
		 * @brief Construct a new node::node object. Each node object is characterized by one or more policy specifying how to handle events
		 * 
		 * @param cls number of classes handled  
		 * @param descriptioin Description of the node
		 * @param g The global mersenne twister random number generator used to generate events' service time
		 */
		node(unsigned int cls, string description, shared_ptr<mt19937_64> g);
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
			for(unsigned int i = 0; i < q.size(); i++)
				len += s.at(i) -> in_queue(); 
			return len;
		}
		// Manage the events in the node
		/**
		 * @brief Handle a new arrival of the event *e* at time *time*.
		 * 
		 * @param time time
		 * @param e *const* pointer to the arriving
		 * @return true if the event is allowed to get in the node, false otherwise
		 */
		bool arrival(shared_ptr<event>& e);
		/**
		 * @brief Handle a departure of the event *e* at time *time*.
		 * 
		 * @param time time
		 * @return Pointer to the departing event
		 */
		shared_ptr<event> departure();
		/**
		 * @brief Get the time of the first event in the EventList
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
		void reset(double time, vector<string> keys = vector<string>(), bool newrun = false) override;
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
				string m = msg.serialize();
				for(shared_ptr<observer> obs: it -> second)
				{
					obs.get()->update(m);
				}
			}
		}
	protected:
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
		 * @brief Holds all the vector observers
		 * 
		 */
        unordered_map<string, list<shared_ptr<observer>>> observable_events;
		/**
		 * @brief keeps the information about attached observers
		 * 
		 */
		int observers;
		/**
		 * @brief Pointer to the global std::random_device
		 * 
		 */
		shared_ptr<mt19937_64> gen;
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
		virtual int schedule(shared_ptr<event>& e, vector<vector<int>> s_map) = 0;
		/**
		 * @brief Chooses one among the available queue for the current job
		 * 
		 * @return an integer used to index the right queue in the q_map structure.
		 * 
		 */
		virtual int enqueue(shared_ptr<event>& e, vector<vector<int>> q_map) = 0;
		/**
		 * @brief Chooses from which queue to pick the next job among the available queues
		 * 
		 * @return an integer used to index the right queue in the s_map structure.
		 */
		virtual int dequeue(shared_ptr<event>& e, vector<vector<int>> s_map) = 0;
	private:
		// ids generator
		inline static unsigned int id_gen = 0;
		// Measures
        vector<int> in;
        vector<int> out;
        vector<double> usage;
        vector<double> last_event;
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
};
#endif