#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <climits>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <functional>
#include <vector>

#include "libdes_const.hpp"
#include "libdes_object.hpp"
#include "libdes_tag.hpp"

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
		 * @param i initial values of information fields, as (tag, value) pairs
		 */
		event(int c, const vector<pair<tag, double>>& i);
		// Get & set methods
		/**
		 * @brief Returns the class the event belong to
		 */
		inline int get_cls() const
		{
			return static_cast<int>(store.value(tags::EVENT_CLS));
		}
		/**
		 * @brief Set the class the event belongs to
		 *
		 * @param c the class's identifier
		 */
		inline void set_cls(int c)
		{
			store.set(tags::EVENT_CLS, static_cast<double>(c));
		}
		/**
		 * @brief Sets the time the event will happen
		 */
		inline void set_time(double time)
		{
			store.set(tags::EVENT_TIME, time);
		}
		/**
		 * @brief Returns the time the event will happen
		 */
		inline double get_time() const
		{
			pair<bool, double> time = store.get(tags::EVENT_TIME);
			if(!time.first)
			{
				throw runtime_error("event::get_time() requesting event end time before than initializing it!");
			}
			return time.second;
		}
		/**
		 * @brief Check what it does
		 */
		inline void set_constraint(double cons)
		{
			store.set(tags::EVENT_CONSTRAINT, cons);
		}
		/**
		 * @brief Check what it does
		 */
		inline pair<bool, double> get_constraint() const
		{
			return store.get(tags::EVENT_CONSTRAINT);
		}
		/**
		 * @brief Returns the value of the info field identified by tag @p t
		 *
		 * @return (true, value) if the field holds a value, (false, 0) otherwise
		 */
		inline pair<bool, double> get_info(tag t) const
		{
			return store.get(t);
		}
		/**
		 * @brief Set the field @p t to @p val, only if it holds no value yet
		 *
		 * @return true if the value was set
		 * @throws invalid_argument for the time and constraint fields, which have dedicated setters
		 */
		inline bool set_info(tag t, double val)
		{
			check_protected(t, "set");
			return store.insert(t, val);
		}
		/**
		 * @brief Returns the tag-indexed information fields, e.g. to build a des::message view
		 */
		inline const tag_store& get_store() const
		{
			return store;
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
		void clone(const event& e);
		/**
		 * @brief Replace the value of the field identified by tag @p t
		 *
		 * @throws invalid_argument for the time and constraint fields, which have dedicated setters
		 */
		inline void emplace_info(tag t, double val)
		{
			check_protected(t, "emplace");
			store.set(t, val);
		}
		/**
		 * @brief Remove the value of the field identified by tag @p t
		 *
		 * @throws invalid_argument for the time and constraint fields
		 */
		inline void remove_info(tag t)
		{
			check_protected(t, "remove");
			store.remove(t);
		}
		/**
		 * @brief Shift back by @p time the event's time and the fields @p keys holding times
		 *
		 * Values that would become negative are set to 0.
		 *
		 * @param time time to subtract
		 * @param keys tags of the additional fields to shift
		 */
		void shift_times(double time, const vector<tag>& keys);
		/**
		 * @brief Reset the time at which the event will happen, see shift_times()
		 * 
		 * @param time time to subtract to the event's time
		 * @param keys tags of the additional fields to shift
		 * 
		 */
		inline void reset(double time, vector<tag> keys = vector<tag>(), bool = false) override
		{
			shift_times(time, keys);
		}
		/**
		 * @brief Remove all the information fields but the identifier; the class is set to 0
		 *
		 */
		void clear() override
		{
			store.clear();
			store.set(tags::EVENT_ID, get_id());
			store.set(tags::EVENT_CLS, 0);
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
		inline bool operator< (const event& rhs) const { return this -> get_time() < rhs.get_time(); }
		inline bool operator> (const event& rhs) const { return this -> get_time() > rhs.get_time(); }
		inline bool operator<=(const event& rhs) const { return !(*this > rhs); }
		inline bool operator>=(const event& rhs) const { return !(*this < rhs); }
		inline bool operator==(const event& rhs) const { return this -> get_time() == rhs.get_time(); }
		inline bool operator!=(const event& rhs) const { return !(*this == rhs); }
	private:
		// ids generator
		inline static atomic<unsigned int> id_gen{0};
		// event's info, indexed by tag
		tag_store store;
		/**
		 * @brief Throws if @p t is the time or the constraint field, which have dedicated setters
		 */
		inline static void check_protected(tag t, const char* op)
		{
			if(t == tags::EVENT_TIME || t == tags::EVENT_CONSTRAINT)
			{
				throw invalid_argument(string("Trying to ") + op + " protected key with event::" + op + "_info(). If it is not a mistake, use event::set_*(double& time) instead. Otherwize, check des/util/util_const.hpp for reserved keys");
			}
		}
};
#endif
