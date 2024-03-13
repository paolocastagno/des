#ifndef COUNTER_H
#define COUNTER_H

#include <iostream>
#include <memory>
#include <vector>

#include "libdes_observer.hpp"
#include "libdes_observable.hpp"
#include "libdes_message.hpp"
#include "libdes_util.hpp"

using namespace std;

namespace des
{
	class counter;
}

class des::counter : public des::observer {
    public:
        /**
         * @brief Construct a new counter object
         * 
         * @param description String identifier of the measure
         * @param cls the number of event classes
         */
        counter(const string& description, int cls) : observer()
        {
            observer_id = description;
            for(unsigned int i = 0; i < cls; i++)
            {
                c.push_back(0);
            }
            c_runs = vector<vector<int>>(cls, vector<int>());
            run = 0;
        }
        /**
         * @brief Updates the counter value
         * 
         */
        inline void update(string m) override
        {
            message msg(m);
            ++c.at(msg.get_value(EVENT_CLS));
        }
        /**
         * @brief Updates the counter value
         * 
         */
        inline void update(int cls)
        {
            ++c.at(cls);
        }
        /**
         * @brief reset the counter of class cls to newval
         * 
         * @param newval 
         * @param cls
         */
        inline void reset(int cls, bool newrun) override
        {
            if(newrun)
            {
                c_runs.at(cls).push_back(c.at(cls));
            }
            c.at(cls) = 0;
        }
        /**
         * @brief reset all the counters to newval
         * 
         * @param newval 
         */
        inline void reset(bool newrun) override
        {
            if(newrun)
            {
                ++run;
            }
            for(unsigned int i = 0; i < c.size(); i++)
            {
                reset(i, newrun);
            }
        }
        /**
         * @brief reset all the counters to initial value (0)
         * 
         */
        inline void clear() override
        {
            for(unsigned int i = 0; i < c.size(); i++)
            {
                c.at(i) = 0;
            }
        }
        /**
         * @brief returns the current value of the counter
         * 
         * @return vector<int> 
         */
        inline int get(int cls)
        {
            return c.at(cls);
        }
        /**
         * @brief 
         * 
         * @return string 
         */
        inline string to_string() const override{
            string str = "";
            for(unsigned int i = 0; i < c.size(); i++)
            {
                str += std::to_string(c.at(i)) + "\t";
            }
            str += "\n";
            return str;
        }
        /**
         * @brief 
         * 
         * @param alpha 
         * @param cls 
         * @return pair<double, double> 
         */
        inline pair<double, double> confidence_interval(double alpha, int cls)
        {
            if(c_runs.size() > 0 && c_runs.at(cls).size() > 1)
            {
                return conf_int(c_runs.at(cls), alpha);
            }
            else
            {
                return pair<double, double>(__DBL_MIN__,__DBL_MAX__);
            }
        }

        inline vector<pair<double,double>> confidence_interval(double alpha)
        {
            vector<pair<double,double>> ci;
            if(c_runs.size() > 0)
            {
                for(int i = 0; i < c_runs.size(); i++)
                {
                    ci.push_back(confidence_interval(alpha,i));
                }
            }
            return ci;
        }

    private:
        /**
         * @brief the counter value
         * 
         */
        vector<int> c;
        /**
         * @brief Run count
         * 
         */
        unsigned int run;
        /**
         * @brief previous runs measures
         * 
         */
        vector<vector<int>> c_runs;
};

#endif