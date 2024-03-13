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
#include <vector>
#include <random>
#include <memory>
#include <unordered_map>

using namespace std;

namespace des
{
	class network;
}

class des::network : public des::observable
{
	public:
		// typedef function<bool(event*, const vector<shared_ptr<node>>&)>Handler;
		// Constructors
		network(){}
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix
		 * @param g pseudo-random number generation
		 * 
		 */
		network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, shared_ptr<mt19937_64>& g);
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix 
		 * @param g pseudo-random number generation
		 * @param hffunc function pointer used to handle forks 
		 * 
		 */
		network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
				int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
				shared_ptr<mt19937_64>& g);
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix 
		 * @param g pseudo-random number generation
		 * @param hbfunc function pointer used to handle blocks at nodes
		 * 
		 */
		network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, 
				pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
										const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
				shared_ptr<mt19937_64>& g);
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix 
		 * @param g pseudo-random number generation
		 * @param hffunc function pointer used to handle forks 
		 * 
		 */
		network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
				int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
				pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
										const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
				shared_ptr<mt19937_64>& g);
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix
		 * @param rtg_failure the routing matrix for jobs not admitted in the destination des::node
		 * @param const_handler unordered_multimap mapping the des::node's index and the des::Constraint::cons_key to the des::event::cls and the function pointer used to handle the des::Constraint
		 * The function does not return a boolean, if true the Constraint is removed, The parameters are:
		 * * const double&: reference to des::Scheduler::time 
		 * * Constraint*: reference to des::Constraint expressing the constraint to the event
		 * * const vector<node>&: reference to des::network::nodes
		 */
		// network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>>  const_handler, shared_ptr<mt19937_64>& g);
		/**
		 * @brief Construct a new network object
		 * 
		 * @param nds vector of nodes
		 * @param rtg the routing matrix
		 * @param rtg_failure the routing matrix for jobs not admitted in the destination des::node
		 * @param const_handler unordered_multimap mapping the des::node's index and the des::Constraint::cons_key to the des::event::cls and the function pointer used to handle the des::Constraint
		 * The function does not return a boolean, if true the Constraint is removed, The parameters are:
		 * * event*: pointer to the des::event associated with the des::Constraint
		 * * Constraint*: reference to des::Constraint expressing the constraint to the event
		 * * const vector<node>&: reference to des::network::nodes
		 */
		// network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg, vector<vector<vector<double>>> rtg_fail, vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>>  const_handler, shared_ptr<mt19937_64>& g);
		/**
		 * @brief Virtual function implementing the routing logic
		 * 
		 * @param e event to route
		 * @param time global clock
		 */
        void route(shared_ptr<event> e, const double& time);
		/**
		 * @brief Get the reference to the next event to handle
		 * 
		 * @return shared_ptr<event> next event
		 * 
		 **/
		shared_ptr<event> next_event();
		/**
		 * @brief Get the number of event routed from source towards destination (in the current run)
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param cls class of events 
		 * 
		 * @return int 
		 */
		// inline int get_count(int source, int destination, int cls)
		// {
		// 	string nm = SIGNAL_NET_ROUTING + "_" + std::to_string(source) + "_" + std::to_string(destination);
		// 	std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(nm);
		// 	if(fnd != observable_events.end())
		// 	{
		// 		//The second element of the list is the observer for the flow
		// 		std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
		// 		return dynamic_cast<counter*>((*lst).get()) -> get(cls);
		// 	}
		// 	else
		// 	{
		// 		throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
		// 	}
		// }
		/**
		 * @brief Get the flow routed from source towards destination (in the current run)
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param cls class of events 
		 * 
		 * @return double 
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
		 * @brief Get the number of event routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param cls class of events 
		 * 
		 * @return int 
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
		 * @brief Get the flow routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param cls class of events 
		 * 
		 * @return double 
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
		 * @brief Get the flow routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param cls class of events 
		 * 
		 * @return double 
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
		 * @brief Get the number of event routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param alpha the confidence level
		 * 
		 * @return vector<int> 
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
		 * @brief Get the number of event routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param alpha the confidence level
		 * 
		 * @return vector<int> 
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
		 * @brief Get the number of event routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param alpha the confidence level
		 * @param cls class of events 
		 * 
		 * @return vector<int> 
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
		 * @brief Get the number of event routed from source towards destination
		 * 
		 * @param source index of the source
		 * @param index of the destintion
		 * @param alpha the confidence level
		 * 
		 * @return vector<int> 
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
		/** s
		 * @brief 
		 * 
		 * @return string 
		 */
		string to_string() const;
		/**
		 * @brief resets the state of the Network
		 * 
		 * @param time amount of time to remove from the current state in order to reset the Network consistently
		 * 
		 */
		void reset(double time, vector<string> keys = vector<string>(), bool newrun = false);
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
		 * @brief Vector with all the des::node in the des::network
		 * 
		 */
        vector<shared_ptr<node>> nodes;
		/**
		 * @brief Tensor describing the routing. For each destination and each event class hosts a double value.
		 * In the hffunc function provided by default, those values are assumed to be the routing probabilities to choose a given destination form the current source.
		 * routing.at(i) is the matrix that describes the routng probability to reach all the nodes inthe network for all the event classes.
		 * routing.at(i).at(j) is the vector describing the routing probability for all the event classes to reach node j leaving from node i
		 * routing.at(i).at().at(k) is the routing proability for an event of class k to get to node  leaving from node i 		 
		 *  
		 */
        vector<vector<vector<double>>> routing;
		/**
		 * @brief const_handler unordered_multimap mapping the des::node's index and the des::Constraint::cons_key to the des::event::cls and the function pointer used to handle the des::Constraint
		 * The function does not return a boolean, if true the Constraint is removed, The parameters are:
		 * * event*: pointer to the des::event associated with the des::Constraint
		 * * Constraint*: reference to des::Constraint expressing the constraint to the event
		 * * const vector<node>&: reference to des::network::nodes
		 * 
		 */
		/**
		 * @brief The inner vector holds the Constraint for a given node an service class   
		 * 
		 */
		// vector<vector<vector<pair<shared_ptr<Constraint>, function<bool(event*, const vector<shared_ptr<node>>&)>>>>> handler;
		/**
		 * @brief Handle events' constarint, both the time dependent and non-time dependent ones
		 * 
		 * @param e
		 * @param time 
		 */
		// void handle_constraints(shared_ptr<event> e, const double& time);
		
		bool insert(shared_ptr<event> e, string node);

		virtual pair<bool, int> hbfunc(shared_ptr<event>, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g);
		virtual int hffunc(shared_ptr<event>, const vector<vector<vector<double>>> &, shared_ptr<mt19937_64>& g);
	private:
		/**
		 * #####################
		 * ## Class variables ##
		 * #####################
		**/
		/**
		 * @brief Reference to the simulator-wide mt1997_64 generator
		 * 
		 */
        shared_ptr<mt19937_64> gen;
		/**
		 * @brief accounts for movements between stations. The mapping correspponds to the one provided with routing failure
		 * 
		 */
        // vector<vector<vector<int>>> count_constrained_events;
		/**
		 * @brief Handle blocking events
		 * 
		 * @param e
		 * @param time
		 * @param routing 
		 */
		pair<bool, int> (*handle_block)(shared_ptr<event>, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>& g);
		/**
		 * @brief Handle blocking events
		 * 
		 * @param e
		 * @param time
		 * @param routing 
		 */
		int (*handle_forks)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&);
};

#endif