#ifndef SAMPLE_H
#define SAMPLE_H

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <optional>
#include <stdexcept>

#include "libdes_observer.hpp"
#include "libdes_observable.hpp"
#include "libdes_message.hpp"
#include "libdes_util.hpp"

using namespace std;

namespace des
{
	class sample;
}

class des::sample : public des::observer {
    public:
        using observer::update;

        /**
         * @brief Construct a sample observing message field @p f, named after it.
         *
         * @param f    Field read from incoming messages (e.g. NODE_SOJOURN).
         * @param cls  Number of event classes to track independently.
         */
        sample(tag f, int cls) : sample(tag_registry::name(f), cls)
        {
            field = f;
        }
        /**
         * @brief Construct a sample fed only with update(value, cls).
         *
         * @param description  Observer identifier.
         * @param cls          Number of event classes to track independently.
         */
        sample(const string& description, int cls) : observer()
        {
            observer_id = description;
            const auto class_count = static_cast<size_t>(cls);
            for(size_t i = 0; i < class_count; i++)
            {
                v.push_back({});
                sum.push_back(0);
            }
            s_runs = vector<vector<double>>(class_count, vector<double>());
            run = 0;
        }
        /**
         * @brief Update directly from a @c des::message object.
         *
         * Appends the observed field to the samples of the class identified
         * by @c EVENT_CLS.
         *
         * @throws logic_error if the sample was built without a field
         *
         * @param msg  Message carrying the measurement value and event class.
         */
        inline void update(const des::message& msg) override{
            if(!field)
            {
                throw logic_error("des::sample " + observer_id + " reads no message field: construct it with a des::tag to observe one");
            }
            update(msg.get_value(*field), static_cast<int>(msg.get_value(tags::EVENT_CLS)));
        }
        /**
         * @brief Update with an explicit value and class index.
         *
         * Bypasses message parsing; appends @p value to the sample store for
         * the given class.
         *
         * @param value  Observed value.
         * @param cls    Event-class index (0-based).
         */
        inline void update(double value, int cls){
            v.at(cls).push_back(value);
            sum.at(cls) += value;
        }
        /**
         * @brief Arithmetic mean of all samples collected so far for class @p cls.
         *
         * @param cls  Event-class index.
         * @return     Sample mean.
         */
        inline double mean(int cls)
        {
            return sum.at(cls) / ((double)v.at(cls).size());
        }
        /**
         * @brief Number of samples collected in the current run for class @p cls.
         *
         * @param cls  Event-class index.
         * @return     Sample count.
         */
        inline unsigned int observations(int cls)
        {
            return v.at(cls).size();
        }
        /**
         * @brief Population standard deviation of all samples in the current run.
         *
         * Returns @c __DBL_MAX__ if fewer than two samples have been collected.
         *
         * @param cls  Event-class index.
         * @return     Standard deviation, or @c __DBL_MAX__ if n < 2.
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
         * @brief End the current run for a single class and optionally start a new one.
         *
         * If @p newrun is @c true, snapshots the current sum into the inter-run
         * store, then clears all samples and resets the running sum.
         *
         * @param cls     Event-class index to reset.
         * @param newrun  When @c true, snapshot before clearing.
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
         * @brief End the current run for all classes and optionally start a new one.
         *
         * Increments the run counter when @p newrun is @c true, then delegates
         * to reset(int, bool) for each class.
         *
         * @param newrun  When @c true, snapshot each class and increment run count.
         */
        inline void reset(bool newrun = false) override
        {
            if(newrun)
            {
                ++run;
            }
            for(size_t i = 0; i < v.size(); i++)
            {
                reset(static_cast<int>(i), newrun);
            }
        }
        /**
         * @brief Discard all samples and reset running sums to zero for all classes.
         *
         * Unlike reset(), does not snapshot the current state and does not clear
         * the inter-run store @c s_runs.
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
         * @brief All collected samples, indexed by class then observation order.
         *
         * @return  Two-dimensional vector: outer index = class, inner = samples in order.
         */
        inline vector<vector<double>> get()
        {
            return v;
        }
        /**
         * @brief Serialise per-class means and standard deviations as a tab-separated string.
         *
         * Each class contributes a tab-separated entry of the form
         * @c "(cls)\tmean(stddev)". Intended for logging and debugging.
         *
         * @return Formatted string of per-class summary statistics.
         */
        inline string to_string() const override{
            string str = "\t" + observer_id;
            for(unsigned int i = 0; i < v.size(); i++)
            {
                str += "\t(" + std::to_string(i) + ")\t" + std::to_string(sum.at(i) / ((double)v.at(i).size())) + "(" + std::to_string(stddev(i)) + ")";
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
                return pair<double, double>(-__DBL_MAX__, __DBL_MAX__);
            }
        }

        inline vector<pair<double,double>> confidence_interval(double alpha)
        {
            vector<pair<double,double>> ci;
            if(s_runs.size() > 0)
            {
                for(size_t i = 0; i < s_runs.size(); i++)
                {
                    ci.push_back(confidence_interval(alpha, static_cast<int>(i)));
                }
            }
            return ci;
        }

    private:
        vector<vector<double>> v;       ///< All samples, per class.
        vector<double>              sum;     ///< Running sum, per class.
        vector<vector<double>> s_runs;  ///< Per-class store of completed-run sums.
        int                              run;     ///< Number of completed runs.
        optional<tag> field;             ///< Field read from messages, if any.
};

#endif
