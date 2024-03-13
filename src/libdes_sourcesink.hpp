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

class des::sourcesink : public des::node
{
	public:
		/**
		 * @brief Construct a new des::SourceSink object representing a sink. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param description String identifier
		 */
		sourcesink(string description) : node::node(description)
		{}

		/**
		 * @brief Construct a new des::SourceSink object representing a source. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		sourcesink(string description, int cls) : node::node(description, cls){};
		/**
		 * @brief Construct a new des::SourceSink object representing a source. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		sourcesink(string description, shared_ptr<mt19937_64> gen) : node::node(description, gen){};
		/**
		 * @brief Construct a new des::SourceSink object representing a source. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param description String identifier
		 * @param cls the number of event classes
		 * @param gen The global std::mt19937_64 pseudo-random generator
		 */
		sourcesink(string description, int cls, shared_ptr<mt19937_64> gen) : node::node(description, cls, gen){};
		/**
		 * @brief Destroy the sourcesink object
		 * 
		 */
		virtual ~sourcesink() {}
		/**
		 * @brief Handle a transition of a job from  the queue to the service  
		 * 
		 */
		// virtual int schedule(shared_ptr<event>& e, std::vector<std::vector<int>> s_map) = 0;
		/**
		 * @brief Chooses one among the available queue for the current job
		 * 
		 * @return an integer used to index the right queue in the q_map structure.
		 * 
		 */
		// virtual int enqueue(shared_ptr<event>& e, std::vector<std::vector<int>> q_map) = 0;
		/**
		 * @brief Chooses from which queue to pick the next job among the available queues
		 * 
		 * @return an integer used to index the right queue in the s_map structure.
		 */
		// virtual int dequeue(shared_ptr<event>& e, std::vector<std::vector<int>> s_map) = 0;
		// Utility methods
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
		 * @brief 
		 * 
		 * @param e 
		 */
		void dispose_event(shared_ptr<event> e)
		{
			events.push_back(e);
		}
		/**
		 * @brief 
		 * 
		 * @param value 
		 */
		void reset(double value, vector<string> keys = vector<string>(), bool newrun = false) override
		{
			node::reset(value, keys, newrun);
		}
		/**
		 * @brief 
		 * 
		 */
		virtual void clear() override
		{
			node::clear();
		}
		/**
		 * @brief 
		 * 
		 * @return string 
		 */
		// virtual string to_string() const = 0;
	protected:
		/**
		 * @brief Get the service object
		 * 
		 * @param cls 
		 * @return double 
		 */
		// virtual double get_service(int& cls) = 0;
	private:
		inline static list<shared_ptr<event>> events = list<shared_ptr<event>>();
};

#endif