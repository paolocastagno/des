#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>

#include "libdes_observer.hpp"
#include "libdes_message.hpp"

using namespace std;

namespace des
{
	class observable;
}

class des::observable
{
	public:
		/**
		 * @brief Construct a new observable object
		 * 
		 */
		observable(){};
		/**
		 * @brief Destroy the observable object
		 * 
		 */
		~observable()
		{
			if(observers != 0)
			{
				detach();
			}
		}
		/**
		 * @brief Construct a new observable object
		 * 
		 * @param id 
		 */
		observable(string id) : sid(id){}
		/**
		 * @brief attach an observer to a given event
		 * 
		 * @param event the string identifier of the signal
		 * @param obs a reference to the observer
		 */
		inline void attach(string event, shared_ptr<observer> obs)
        {
			unordered_map<string, list<shared_ptr<observer>>>::iterator it = observable_events.find(event);
			if(it != observable_events.end())
			{ 
				it -> second.push_back(obs);
				obs -> set_attached(true);
				obs -> set_event(event);
				++observers;
			}
			else{
				throw invalid_argument("Measurable event " + obs -> get_event() + " is not defined in network " + get_sid());
			}
        }
		/**
		 * @brief detach all measures from the current entity
		 * 
		 * @param description the string identifier of the measure
		 */
        inline void detach()
        {
			for(unordered_map<string, list<shared_ptr<observer>>>::iterator it = observable_events.begin(); it != observable_events.end(); it++)
			{
				while(!it -> second.empty())
				{
					shared_ptr<observer> o = *(it -> second.begin());
					o -> set_attached(false);
					it -> second.pop_front();
					--observers;
				}
			}
        }
		/**
		 * @brief detach a measure from the current entity
		 * 
		 * @param description the string identifier of the measure
		 */
        inline void detach(string measure, string event)
        {
			unordered_map<string, list<shared_ptr<observer>>>::iterator it = observable_events.find(event);
			if(it != observable_events.end())
			{
				list<shared_ptr<observer>>::iterator i = it -> second.begin();
				while(i != it -> second.end() && i ->get() -> get_observer() != measure)
				{
					i++;
				}
				if(i != it -> second.end())
				{
					it -> second.erase(i);
					--observers;
				}
			}
        }
		/**
         * @brief notify all attached observers
         * 
		 * @param signal the string identifier of the signal
         */
        virtual void notify(string signal, message& message) = 0;
		/**
		 * @brief Get the sid object
		 * 
		 * @return string 
		 */
		inline string get_sid() const
		{
			return sid;
		}
		/**
		 * @brief Set the sid object
		 * 
		 * @param stringid 
		 */
		inline void set_sid(string stringid)
		{
			sid = stringid;
		}
	protected:
		string sid;
		/**
		 * @brief keeps the information about attached observers
		 * 
		 */
		int observers;
		/**
		 * @brief Holds all the vector observers
		 * 
		 */
        unordered_map<string, list<shared_ptr<observer>>> observable_events;
};
#endif