#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <climits>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <functional>

#include "libdes_const.hpp"
#include "libdes_object.hpp"

using namespace std;

namespace des
{
	class event;
}

class des::event : public des::object
{
	public:
		/* ~~~~~~~~~~~~~~~~~~~~~~~~ *
		* Constructors & destructor *
		* ~~~~~~~~~~~~~~~~~~~~~~~~~ *
		*/
		/**
		 * @brief Construct a new event
		 * 
		 * @param c event's class
		 * @param i map containing additioal informations about the event
		 */
		event();
		/**
		 * @brief Construct a new event
		 * 
		 * @param c event's class
		 */
		event(int c);
		/**
		 * @brief Construct a new event
		 * 
		 * @param c event's class
		 * @param i map containing additioal informations about the event
		 */
		event(int c, unordered_map<string,double> i);
		// Get & set methods
		/**
		 * @brief Returns the identifier of the event, which is unique across all the generated events
		 */
		// inline int get_id() const
		// {
		// 	return id;
		// }
		/**
		 * @brief Returns the class the event belong to
		 */
		inline int get_cls() const
		{
			return cls;
		}
		/**
		 * @brief Set the class the event belongs to
		 * 
		 * @param c the class's identifier
		 */
		inline void set_cls(int c)
		{
			cls = c;
			set_info(EVENT_CLS, cls);
		}
		/**s
		 * @brief Sets the time the event will happen
		 */
		inline void set_time(double time)
		{
			info[EVENT_TIME] = time;
		}
		/**
		 * @brief Returns the time the event will happen
		 */
		inline double get_time() const
		{
			auto time_it = info.find(EVENT_TIME);
			if( time_it == info.end())
				throw runtime_error("event::get_time() requesting event end time before than initializing it!");
			else
			{
				return time_it -> second;
			}
		}
		/**
		 * @brief Check what it does
		 */
		inline void set_constraint(double cons)
		{
			info[EVENT_CONSTRAINT] = cons;
		}
		/**
		 * @brief Check what it does
		 */
		inline pair<bool, double> get_constraint()
		{
			auto it = info.find(EVENT_CONSTRAINT);
			if( it == info.end())
				return pair<bool,double>(false, 0);
			else
			{
				return pair<bool,double>(true, it -> second);
			}
		}
		/**
		 * @brief Returns the value of the info field corresponding to the key passed as parameter
		 * 
		 * @param name string identifying the information required
		 */
		inline pair<bool, double> get_info(string name) const
		{
			auto it = info.find(name);
			if( it == info.end())
				return pair<bool,double>(false, 0);
			else
			{
				return pair<bool,double>(true, it -> second);
			}
		}
		/**
		 * @brief Set an entry in the information field to host the key-value pair passed as parameter
		 * 
		 * @param name key used to retrive the value
		 * @param val the value to store
		 * 
		 */
		inline bool set_info(string name, double val)
		{
			if(EVENT_TIME== name || EVENT_CONSTRAINT== name)
			{
				throw invalid_argument("Trying to set protected key with event::set_info(). If it is not a mistake, use event::set_*(double& time) instead. Otherwize, check des/util/util_const.hpp for reserved keys");
			}
			pair<unordered_map<string,double>::iterator,bool> in = info.insert(make_pair(name, val));
			return in.second;
		}
		/**
		 * @brief Get the unordered map info
		 * 
		 * @return unordered_map<string, double> 
		 */
		inline unordered_map<string, double> get_map_info()
		{
			return info;
		}
		/* ~~~~~~~~~~~~~~~ *
		*  Utility methods *
		* ~~~~~~~~~~~~~~~~ *
		*/
		/**
		 * @brief Copies all the attributes of the event e into the current one
		 * 
		 * @param e the event to clone (all the fileds will be coped but the identifier)
		 * 
		 */
		void clone(event e);
		/**
		 * @brief Replace the value corresponding to the key passed as parameter
		 * 
		 * @param name key used to retrive the value
		 * @param val the value to store
		 * 
		 */
		void emplace_info(string name, double val);
		/**
		 * @brief Remove the entry corresponding to the key passed as parameter
		 * 
		 * @param name key used to retrive the value
		 * @param val the value to store
		 * 
		 */
		void remove_info(string name);
		/**
		 * @brief Reset the time at which the event will happen  
		 * 
		 * @param delta time to subtract to the event's time
		 * 
		 */
		inline void reset(double time, std::vector<string> keys = std::vector<string>(), bool newrun = false) override
		{
			double t = get_time();
			set_time(t > time? t - time : 0);
			for(string str: keys)
			{
				pair<bool, double> value = get_info(str);
				if(value.first)
				{
					emplace_info(str, value.second > time? value.second - time : 0);
				}
			} 
		}
		/**
		 * @brief Replace the value corresponding to the key passed as parameter
		 * 
		 * @param name key used to retrive the value
		 * @param val the value to store
		 * 
		 */
		void clear() override
		{
			cls = 0;
			info.clear();
			set_info(EVENT_ID, get_id());
		}
		/**
		 * @brief Check wheter the event has already been initilized or not
		 * 
		 */
		bool is_initialized() const;
		/**
		 * @brief Serialize the event to a string
		 * 
		 */
		string to_string() const override;
		/* ~~~~~~~~~~~~~~~~~~~~ *
		*  Comparison operators *
		*  ~~~~~~~~~~~~~~~~~~~~ *
		*/
		inline bool operator< (const event& rhs){ return this -> get_time() < rhs.get_time(); }
		inline bool operator> (const event& rhs){ return this -> get_time() > rhs.get_time(); }
		inline bool operator<=(const event& rhs){ return !(*this > rhs); }
		inline bool operator>=(const event& rhs){ return !(*this < rhs); }
		inline bool operator==(const event& rhs){ return this -> get_time() == rhs.get_time(); }
		inline bool operator!=(const event& rhs){ return !(*this == rhs); }
	private:
		// ids generator
		inline static unsigned int id_gen = 0;
		// event's class
		int cls;
		// event's info
		unordered_map<string, double> info;
};
#endif
