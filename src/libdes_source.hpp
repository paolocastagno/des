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
	class source;
}

class des::source : public des::sourcesink
{
	public:
		/**
		 * @brief Construct a new des::SourceSink object representing a source. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		source(std::vector<double> r, string description, shared_ptr<mt19937_64> gen);
		/**
		 * @brief Construct a new des::SourceSink object representing a sink. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		source(string description);
		/**
		 * @brief Handle a transition of a job from  the queue to the service  
		 * 
		 */
		int schedule(shared_ptr<event>& e, std::vector<std::vector<int>> s_map) override;
		/**
		 * @brief Chooses one among the available queue for the current job
		 * 
		 * @return an integer used to index the right queue in the q_map structure.
		 * 
		 */
		int enqueue(shared_ptr<event>& e, std::vector<std::vector<int>> q_map) override;
		/**
		 * @brief Chooses from which queue to pick the next job among the available queues
		 * 
		 * @return an integer used to index the right queue in the s_map structure.
		 */
		int dequeue(shared_ptr<event>& e, std::vector<std::vector<int>> s_map) override;
		// Utility methods
		/**
		 * @brief 
		 * 
		 * @param value 
		 */
		inline void reset(double value, vector<string> keys = vector<string>(), bool newrun = false) override
		{
			this -> sourcesink::reset(value, keys);
		}
		/**
		 * @brief 
		 * 
		 */
		inline void clear() override
		{
			this -> sourcesink::clear();
		}
		/**
		 * @brief Set the rate object
		 * 
		 * @param rate 
		 * @param cls 
		 */
		inline void set_rate(double rate, int cls)
		{
			exp.at(cls) = exponential_distribution<double>(rate);
		}
		/**
		 * @brief 
		 * 
		 * @return string 
		 */
		string to_string() const override;
	protected:
		/**
		 * @brief Get the service object
		 * 
		 * @param cls 
		 * @return double 
		 */
		double get_service(unsigned int& cls, unsigned int& idx) override;
	private:
        std::vector<double> rate;
        std::vector<exponential_distribution<double>> exp;
};
#endif