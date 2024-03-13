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

class des::sink : public des::sourcesink
{
	public:
		/**
		 * @brief Construct a new des::SourceSink object representing a source. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		sink(std::vector<double> r, string description, mt19937_64& gen);
		/**
		 * @brief Construct a new des::SourceSink object representing a sink. Every SourceSink object handles events according to a FIFO queue and a SingleServer server.
		 * 
		 * @param r Vector specifying per class service rates 
		 * @param description String identifier
		 * @param rdev The global std::mt19937_64 pseudo-random generator
		 */
		sink(string description, int cls);
		/**
		 * @brief Destroy the sink object
		 * 
		 */
		sink(string description);
		/**
		 * @brief Destroy the sink object
		 * 
		 */
		~sink();
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
			// Write the code to clear the observers
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
		 * @brief 
		 * 
		 * @return string 
		 */string to_string() const override;
	protected:
		/**
		 * @brief Get the service object
		 * 
		 * @param cls 
		 * @return double 
		 */
		inline double get_service(unsigned int& cls, unsigned int& idx) override
		{
			return 0;
		}
	private:
        std::vector<double> rate;
        std::vector<exponential_distribution<double>> exp;
};
#endif