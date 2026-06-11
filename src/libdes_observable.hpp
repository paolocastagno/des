#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <iostream>
#include <list>
#include <stdexcept>
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
			 * @brief Construct an observable with no string identifier.
			 *
			 * Derived classes are expected to populate @c observable_events with the
			 * signals they support before clients call attach(). The observer count is
			 * initialized to zero so attach()/detach() can maintain it consistently.
			 */
			observable() : sid(), observers(0) {};
			/**
			 * @brief Destroy the observable object.
			 *
			 * All attached observers are detached before destruction so their attached
			 * flags do not continue to point conceptually at a dead observable.
			 */
		~observable()
		{
			if(observers != 0)
			{
				detach();
			}
		}
			/**
			 * @brief Construct an observable with a string identifier.
			 *
			 * @param id Human-readable identifier used in diagnostics and error messages.
			 */
			observable(string id) : sid(id), observers(0){}
			/**
			 * @brief Attach an observer to a registered signal.
			 *
			 * The observable keeps a shared reference to @p obs in the list associated
			 * with @p event. The signal must already exist in @c observable_events;
			 * derived classes usually create those entries in their constructors.
			 *
			 * @param event String identifier of the signal to observe.
			 * @param obs Observer instance that will receive notifications.
			 * @throws invalid_argument if @p event is not registered by this observable.
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
			 * @brief Detach every observer from every registered signal.
			 *
			 * Each observer's attached flag is cleared as it is removed, and the total
			 * observer count is decremented for every detached observer.
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
			 * @brief Detach one observer from a signal by observer identifier.
			 *
			 * If the signal or observer identifier is not found, the method leaves the
			 * observable unchanged.
			 *
			 * @param measure Observer identifier, as returned by observer::get_observer().
			 * @param event Signal identifier from which the observer should be removed.
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
	         * @brief Notify all observers attached to a signal.
	         *
	         * Concrete observables decide how to deliver @p message to the observers
	         * associated with @p signal.
	         *
			 * @param signal String identifier of the signal being emitted.
			 * @param message Payload delivered to observers.
	         */
	        virtual void notify(string signal, message& message) = 0;
			/**
			 * @brief Return the observable string identifier.
			 *
			 * @return Human-readable identifier for diagnostics and reporting.
			 */
		inline string get_sid() const
		{
			return sid;
		}
			/**
			 * @brief Set the observable string identifier.
			 *
			 * @param stringid Human-readable identifier for diagnostics and reporting.
			 */
		inline void set_sid(string stringid)
		{
			sid = stringid;
		}
	protected:
		string sid;
			/**
			 * @brief Total number of observer registrations across all signals.
			 *
			 * The same observer attached to multiple signals contributes one count per
			 * registration.
			 */
			int observers;
			/**
			 * @brief Registered signals and their attached observer lists.
			 *
			 * Keys are signal identifiers. Values are ordered lists of observers that
			 * should be notified when the corresponding signal is emitted.
			 */
        unordered_map<string, list<shared_ptr<observer>>> observable_events;
};
#endif
