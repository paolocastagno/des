#ifndef STATION_H
#define STATION_H

#include "libdes_node.hpp"
#include "libdes_event.hpp"
#include "libdes_policy.hpp"
#include "libdes_fifo.hpp"
#include "libdes_is.hpp"
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
	template <typename TT, template <typename> typename T> class station;
}

template <typename TT, template <typename> typename T> class des::station : public des::node
{
	public:
		// Constructors & destructor
		/**
		 * @brief Construct a new des::station object. Every station object handles events according to a FIFO queue and a SingleServer server
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param gen The global std::mt19937 used to generate pseudo-random numbers
		 */
		station(vector<vector<shared_ptr<T<TT>>>> rand_dist, unsigned int nserver, unsigned int s_places, shared_ptr<policy> p_server, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Construct a new des::station object. Every station object handles events according to a FIFO queue and a SingleServer server
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param gen The global std::mt19937 used to generate pseudo-random numbers
		 */
		station(vector<vector<shared_ptr<T<TT>>>> rand_dist, unsigned int nserver, unsigned int s_places, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Construct a new des::station object. Every station object handles events according to a FIFO queue and a SingleServer server
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param gen The global std::mt19937 used to generate pseudo-random numbers
		 */
		station(vector<vector<shared_ptr<T<TT>>>> rand_dist, unsigned int nserver, unsigned int s_places, unsigned int nqueue, unsigned int q_places, shared_ptr<policy> p_queue, shared_ptr<policy> p_server, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Construct a new des::station object. Every station object handles events according to a FIFO queue and a SingleServer server
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param gen The global std::mt19937 used to generate pseudo-random numbers
		 */
		station(vector<vector<shared_ptr<T<TT>>>> rand_dist, unsigned int nserver, unsigned int s_places, unsigned int nqueue, unsigned int q_places, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Construct a new des::station object. Every station object handles events according to a FIFO queue and a SingleServer server
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param gen The global std::mt19937 used to generate pseudo-random numbers
		 */
		station(vector<vector<shared_ptr<T<TT>>>> rand_dist, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Sets the random variable parameters
		 * 
		 * @param params 
		 */
		shared_ptr<T<TT>> get_rng(unsigned int& cls, unsigned int& idx);
		/**
		 * @brief TBDS
		 * 
		 * @return string 
		 */
		string to_string() const override;
		/**
		 * @brief Get des::station service time
		 * 
		 * @param cls Identifyes the class of the des::Event requesting the service. Each service class has its own service rate 
		 * @return double 
		 */
		double get_service(unsigned int& cls, unsigned int& idx) override;
		/**
		 * @brief Returns the index of the queue where to equeue the job *e*
		 * 
		 * @param e the event to enqueue
		 * @param q_map the mapping between queue and event class: it shows which event's classes can be handled by a specific queue
		 *  
		 * @return int the index 
		 */
		int enqueue(shared_ptr<event>& e, vector<vector<int>> q_map) override;
		/**
		 * @brief Returns the index of the queue where to dequeue the next job to handle
		 * 
		 * @param e the event to enqueue
		 * @param q_map the mapping between queue and event class: it shows which event's classes can be handled by a specific queue* @return double 
		 *  
		 * @return int the index 
		 */
		int dequeue(shared_ptr<event>& e, vector<vector<int>> q_map) override;
		/**
		 * @brief Returns the index of the server where to schedule the next event
		 * 
		 * @param e the event to process
		 * @param q_map the mapping between queue and event class: it shows which event's classes can be handled by a specific queue* @return double 
		 *  
		 * @return int the index 
		 */
		int schedule(shared_ptr<event>& e, vector<vector<int>> s_map) override;
	private:
		/**
		 * @brief Data structure of size m * n, where m are the number of server in the queue and n is the number of jobs' classes
		 * The element rng[i,j] is the service time distribution at the i-th server for the j-th jobs' class
		 * 
		 */
        vector<vector<shared_ptr<T<TT>>>> rng;
		bool measure_s;
};
#endif