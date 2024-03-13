#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

#include "libdes_observer.hpp"
#include "libdes_observable.hpp"
#include "libdes_message.hpp"
#include "libdes_util.hpp"

using namespace std;

namespace des
{
	class vector;
}

class des::vector : public des::observer {
    public:
        /**
         * @brief Construct a new vector object
         * 
         * @param description String identifier of the observer
         */
        vector(const string& description, int cls) : observer() // description, id_gen++){}
        {
            observer_id = description;
            for(unsigned int i = 0; i < cls; i++)
            {
                v.push_back({});
                sum.push_back(0);
            }
            s_runs = std::vector<std::vector<double>>(cls, std::vector<double>());
            run = 0;
        }
        /**
         * @brief Updates the vector value
         * 
         */
        inline void update(string m) override{
            message msg(m);
            v.at(msg.get_value(EVENT_CLS)).push_back(msg.get_value(observer_id));
            sum.at(msg.get_value(EVENT_CLS)) += msg.get_value(observer_id);
        }
        /**
         * @brief Updates the vector value
         * 
         */
        inline void update(double value, int cls){
            v.at(cls).push_back(value);
            sum.at(cls) += value;
        }
        /**
         * @brief Computes the mean for a class
         * 
         * @param cls 
         * @return double 
         */
        inline double mean(int cls)
        {
            return sum.at(cls) / ((double)v.at(cls).size());
        }
        /**
         * @brief Returns the number of observations
         * 
         * @param cls 
         * @return double 
         */
        inline unsigned int observations(int cls)
        {
            return v.at(cls).size();
        }
        /**
         * @brief Computes the Standard deviation for a class
         * 
         * @param cls 
         * @return double 
         */
        inline double stddev(int cls) const
        {
            double sdev = __DBL_MAX__;
            if(v.at(cls).size() >= 2)
            {
                sdev = 0;
                double mean = sum.at(cls) / ((double)v.at(cls).size());
                for(unsigned int j = 0; j < v.at(cls).size(); j++)
                {
                    sdev += pow(mean-v.at(cls).at(j),2);
                }
                sdev = sqrt(sdev / ((double)v.at(cls).size()));
            }
            return sdev;
        }
        /**
         * @brief Resets the vector for the given class and value
         * 
         * @param value 
         * @param cls 
         */
        inline void reset(int cls, bool newrun) override
        {
            if(newrun)
            {
                s_runs.at(cls).push_back(sum.at(cls));
            }
            sum.at(cls) = 0;
            v.at(cls).clear();
        }
        /**
         * @brief Resets the vector for the given class and value
         * 
         * @param value 
         * @param cls 
         */
        inline void reset(bool newrun = false) override
        {
            if(newrun)
            {
                ++run;
            }
            for(int i = 0; i < v.size(); i++)
            {
                reset(i, newrun);
            }
        }
        /**
         * @brief removes all the elements from the structure
         * 
         */
        inline void clear() override
        {
            for(unsigned int i = 0; i < v.size(); i++)
            {
                v.at(i).clear();
                sum.at(i) = 0;
            }
        }
        /**
         * @brief returns the current value of the vector
         * 
         * @return double 
         */
        inline std::vector<std::vector<double>> get()
        {
            return v;
        }
        /**
         * @brief Writes the relevant information about the observer to string
         * 
         * @return string 
         */
        inline string to_string() const override{
            string str = "\t" + observer_id;
            for(unsigned int i = 0; i < v.size(); i++)
            {
                str = "\t(" + std::to_string(i) + ")\t" + std::to_string(sum.at(i) / ((double)v.at(i).size())) + "(" + std::to_string(stddev(i)) + ")";
            }
            return str;
        }
        inline pair<double, double> confidence_interval(double alpha, int cls)
        {
            if(s_runs.size() > 0 && s_runs.at(cls).size() > 1)
            {
                return conf_int(s_runs.at(cls), alpha);
            }
            else
            {
                return pair<double, double>(__DBL_MAX__,__DBL_MAX__);
            }
        }

        inline std::vector<pair<double,double>> confidence_interval(double alpha)
        {
            std::vector<pair<double,double>> ci;
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
         * @brief the vector value
         * 
         */
        std::vector<std::vector<double>> v;
        std::vector<double> sum;
        std::vector<std::vector<double>> s_runs;
        int run;
};

#endif