#ifndef IS_H
#define IS_H

#include <list>
#include <memory>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class is;
}

class des::is : public des::policy
{
	public:
		/**
		 * @brief Construct a new Infinite Servre (is) object
		 * 
		 */
		is();
		/**
		 * @brief 
		 * 
		 * @param e 
		 * @param l 
		 * @param positions 
		 * @param rate 
		 * @return true 
		 * @return false 
		 */
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double rate);
		/**
		 * @brief 
		 * 
		 * @param e 
		 * @param l 
		 * @param positions 
		 * @return true 
		 * @return false 
		 */
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions);
		/**
		 * @brief 
		 * 
		 * @param e 
		 * @param l 
		 * @param rate 
		 * @return true 
		 * @return false 
		 */
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, double rate);
		/**
		 * @brief 
		 * 
		 * @return true 
		 * @return false 
		 */
		bool front();
};
#endif