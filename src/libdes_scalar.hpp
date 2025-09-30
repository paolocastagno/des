#ifndef SCALAR_H
#define SCALAR_H

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
	class scalar;
}

class des::scalar : public des::observer {
    public:
        /**
         * @brief Construct a new scalar object
         * 
         * @param description String identifier of the measure
         * @param cls the number of event classes
         */
        scalar(const string& description, int cls) : observer()
        {
            observer_id = description;
            for(unsigned int i = 0; i < cls; i++)
            {
                v.push_back(0);
                n.push_back(0);
                s.push_back(0);
                a.push_back(0);
                q.push_back(0);
                obs.push_back(0);
            }
            s_runs = vector<vector<double>>(cls, vector<double>());
            run = 0;
        }
        /**
         * @brief Updates the scalar value
         * 
         */
        inline void update(string m) override
        {
            message msg(m);
            int cls = static_cast<int>(msg.get_value(EVENT_CLS));
            v.at(cls) += msg.get_value(observer_id);
            n.at(cls) += 1;
            // cout << m << " --- " << msg.get_value(observer_id) << endl;
            // cout << "v(" << cls << "): " << v.at(cls) << " n: " << n.at(cls) << " msg[" << observer_id << "] " << msg.get_value(observer_id) << endl;
        }
        /**
         * @brief Updates the scalar value
         * 
         */
        inline void update(double s, int cls)
        {
            v.at(cls) += s;
            n.at(cls) += 1;
        }
        /**
         * @brief Compute population standard deviation according to algorithm presented here (https://en.wikipedia.org/wiki/Standard_deviation)
         * 
         * @param time 
         * @param cls 
         * @return double 
         */
        inline double stddev(int cls, double normalization = 1.0)
        {
            if(v.at(cls) == 0)
            {
                return __DBL_MAX__;
            }
            double prev_a = a.at(cls);
            double value = v.at(cls)/normalization;
            obs.at(cls) += 1;
            s.at(cls) = s.at(cls) + (value/static_cast<double>(n.at(cls))-s.at(cls))/static_cast<double>(obs.at(cls));
            a.at(cls) = a.at(cls) + (value/static_cast<double>(n.at(cls)) - a.at(cls))/static_cast<double>(obs.at(cls));
            q.at(cls) = q.at(cls) + (value/static_cast<double>(n.at(cls))-prev_a)*(value/static_cast<double>(n.at(cls))-a.at(cls));
            if(obs.at(cls) < 2)
            {
                return __DBL_MAX__;
            }
            else
            {
                return q.at(cls)/static_cast<double>(obs.at(cls));
            }
        }
        /**
         * @brief reset the scalar of class cls to newval
         * 
         * @param newval 
         * @param cls
         */
        inline void reset(int cls, bool newrun) override
        {
            if(newrun)
            {
                double scval = static_cast<double>(s.at(cls)); // /static_cast<double>(obs.at(cls));
                if(!isnan(scval))
                    s_runs.at(cls).push_back(scval);
            }
            v.at(cls) = 0;
            n.at(cls) = 0;
            s.at(cls) = 0;
            a.at(cls) = 0;
            q.at(cls) = 0;
            obs.at(cls) = 0;
        }
        /**
         * @brief reset all the scalars to newval
         * 
         * @param newval 
         */
        inline void reset(bool newrun) override
        {
            if(newrun)
            {
                ++run;
            }
            for(unsigned int i = 0; i < s.size(); i++)
            {
                if(newrun)
                {
                    s_runs.at(i).push_back(s.at(i));
                }
                v.at(i) = 0;
                n.at(i) = 0;
                s.at(i) = 0;
                a.at(i) = 0;
                q.at(i) = 0;
                obs.at(i) = 0;
            }
        }
        /**
         * @brief reset all the scalars to initial value (0)
         * 
         */
        inline void clear() override
        {
            for(unsigned int i = 0; i < s.size(); i++)
            {
                v.at(i) = 0;
                n.at(i) = 0;
                s.at(i) = 0;
                a.at(i) = 0;
                q.at(i) = 0;
                obs.at(i) = 0;
            }
        }
        /**
         * @brief returns the current value of the scalar
         * 
         * @return vector<int> 
         */
        inline double get(int cls)
        {
            return s.at(cls);
        }
        /**
         * @brief Get the scalar object
         * 
         * @param cls 
         * @return double 
         */
        inline double get_scalar(int cls)
        {
            double sum = 0;
            for(double val: s_runs.at(cls))
            {
                sum += val;
            }
            return sum/static_cast<double>(run);
        }
        /**
         * @brief Get the average value of the scalarfor the current run
         * 
         * @param cls 
         * @return double 
         */
        inline double run_avg(int cls)
        {
            return v.at(cls)/static_cast<double>(n.at(cls));
        }
        /**
         * @brief 
         * 
         * @return string 
         */
        inline string to_string() const override{
            string str = "";
            for(unsigned int i = 0; i < s.size(); i++)
            {
                str += std::to_string(s.at(i)) + "\t";
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
            if(s_runs.size() > 0 && s_runs.at(cls).size() > 1)
            {
                return conf_int(s_runs.at(cls), alpha);
            }
            else
            {
                return pair<double, double>(__DBL_MIN__,__DBL_MAX__);
            }
        }

        inline vector<pair<double,double>> confidence_interval(double alpha)
        {
            vector<pair<double,double>> ci;
            if(s_runs.size() > 0)
            {
                for(int i = 0; i < s_runs.size(); i++)
                {
                    ci.push_back(confidence_interval(alpha,i));
                }
            }
            return ci;
        }

    private:
        /**
         * @brief the actual value of the scalar (the sum of all updates) 
         * 
         */
        vector<double> v;
        /**
         * @brief the number of updates
         * 
         */
        vector<int> n;
        /**
         * @brief the scalar value (given by observation of the actual scalar's value)
         * 
         */
        vector<double> s;
        /**
         * @brief the number of observations
         * 
         */
        vector<int> obs;
        /**
         * @brief Accumulates the sum. This variable is used to compute the standard deviation
         * 
         */
        vector<double> a;
        /**
         * @brief Accumulates the squared sum. This variable is used to compute the standard deviation
         * 
         */
        vector<double> q;
        /**
         * @brief Run count
         * 
         */
        unsigned int run;
        /**
         * @brief previous runs measures
         * 
         */
        vector<vector<double>> s_runs;
};

#endif