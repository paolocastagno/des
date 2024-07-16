#ifndef OBSERVER_H
#define OBSERVER_H

#include <string>
#include <memory>

#include "libdes_observable.hpp"

using namespace std;

namespace des
{
	class observer;
}

class des::observer
{
    public:
        /**
         * @brief Construct a new measure object
         * 
         */
        observer() : attached(false), id(id_gen++) {};
        /**
         * @brief Destroy the measure object
         * 
         */
        virtual ~observer(){}
        /**
         * @brief Get the attached object
         * 
         * @return true 
         * @return false 
         */
        inline bool get_attached() const
        {
            return attached;
        }
        /**
         * @brief Get the observer object descritption
         * 
         * @return string 
         */
        inline string get_observer() const{
            return observer_id;
        }
        /**
         * @brief Get the observable's event string identifier
         * 
         * @return string 
         */
        inline string get_event() const{
            return event;
        }
        /**
         * @brief Get the id object
         * 
         * @return unsigned int 
         */
        inline unsigned int get_id() const{
            return id;
        }
        /**
         * @brief Set the attached object
         * 
         * @param a 
         */
        inline void set_attached(bool a)
        {
            attached = a;
        }
        /**
         * @brief Set the observable's event string identifier
         * 
         * @param obs_id 
         */
        inline void set_event(const string& evnt)
        {
            event = evnt;
        }
        /**
         * @brief updates the state of the observer according to the received message
         * 
         * @param message 
         */
        virtual void update(string message) = 0;
        /**
         * @brief Check wheter the observer is associated with an observable entity
         * 
         * @return true 
         * @return false 
         */
        inline bool is_attached()
        {
            return attached;
        }
        /**
         * @brief resets the state of the observer according to the parameter value for all the obs' classes
         * 
         * @param value 
         */
        virtual void reset(bool newrun = false) = 0;
        /**
         * @brief resets the state of the observer according to the parameter value for the given class
         * 
         * @param value 
         * @param cls 
         */
        virtual void reset(int cls, bool newrun = false) = 0;
        /**
         * @brief Resets the state of the observer
         * 
         */
        virtual void clear() = 0;
        /**
		 * @brief 
		 * 
		 * @return string 
		 */
		virtual string to_string() const = 0;
    protected:
        /**
         * @brief keeps the association state
         * 
         */
        bool attached;
        /**
		 * @brief Observer string identifier 
		 * 
		 */
		string observer_id;
        /**
		 * @brief Observable event string identifier 
		 * 
		 */
		string event;
        unsigned int id;
        inline static unsigned int id_gen = 0;
};

#endif
