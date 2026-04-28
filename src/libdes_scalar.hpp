#ifndef SCALAR_H
#define SCALAR_H

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
	class scalar;
}

/**
 * @brief Running-mean observer for numeric measurements, supporting multiple event
 *        classes and multiple simulation replications.
 *
 * Each update feeds Welford's online algorithm, keeping a numerically stable
 * running mean and M2 accumulator for the *current* run.  When a run ends,
 * reset(true) snapshots the run mean into an internal store so that
 * cross-run statistics (inter-run stddev, confidence intervals) can be
 * computed once all replications are complete.
 *
 * Two standard-deviation views are provided:
 *   - run_stddev()  — within the ongoing run (intra-run convergence).
 *   - stddev()      — across completed run means (inter-run convergence).
 */
class des::scalar : public des::observer {
    public:
        /**
         * @brief Construct a new scalar observer.
         *
         * @param description  Unique string identifier used both as the observer ID
         *                     and as the key read from incoming messages.
         * @param cls          Number of event classes to track independently.
         */
        scalar(const string& description, int cls) : observer()
        {
            observer_id = description;
            for(unsigned int i = 0; i < cls; i++)
            {
                v.push_back(0);
                n.push_back(0);
                a.push_back(0.0);
                q.push_back(0.0);
            }
            s_runs = vector<vector<double>>(cls, vector<double>());
            run = 0;
        }
        /**
         * @brief Update from a serialised message string.
         *
         * The string is deserialised into a @c des::message; the value keyed by
         * @c observer_id is fed to the Welford accumulator for the class
         * identified by @c EVENT_CLS.
         *
         * @param m  Serialised message string.
         */
        inline void update(string m) override
        {
            message msg(m);
            welford_update(msg.get_value(observer_id),
                           static_cast<int>(msg.get_value(EVENT_CLS)));
        }
        /**
         * @brief Update directly from a @c des::message object.
         *
         * Avoids the serialisation round-trip of the string overload.
         * Reads @c observer_id and @c EVENT_CLS from the message.
         *
         * @param msg  Message carrying the measurement value and event class.
         */
        inline void update(const des::message& msg) override
        {
            welford_update(msg.get_value(observer_id),
                           static_cast<int>(msg.get_value(EVENT_CLS)));
        }
        /**
         * @brief Update with an explicit value and class index.
         *
         * Bypasses message parsing; useful when the calling code already holds
         * the numeric value and class.
         *
         * @param s    Observed value.
         * @param cls  Event-class index (0-based).
         */
        inline void update(double s, int cls)
        {
            welford_update(s, cls);
        }
        /**
         * @brief Intra-run sample standard deviation (Bessel-corrected), computed
         *        online via Welford's algorithm alongside every update() call.
         *        Use to assess convergence within the current run.
         *        Returns __DBL_MAX__ if fewer than two samples have been collected.
         */
        inline double run_stddev(int cls) const
        {
            if(n.at(cls) < 2)
                return __DBL_MAX__;
            return sqrt(q.at(cls) / static_cast<double>(n.at(cls) - 1));
        }
        /**
         * @brief Inter-run standard deviation of the per-run means.
         *        Use to assess convergence across replications.
         *        Returns __DBL_MAX__ if fewer than two runs have been completed.
         */
        inline double stddev(int cls) const
        {
            if(s_runs.at(cls).size() < 2)
                return __DBL_MAX__;
            double m = 0.0;
            for(double val : s_runs.at(cls))
                m += val;
            m /= static_cast<double>(s_runs.at(cls).size());
            return ::stddev(s_runs.at(cls), m);
        }
        /**
         * @brief End the current run for a single class and optionally start a new one.
         *
         * If @p newrun is @c true and at least one sample was collected, the
         * current-run mean is appended to the inter-run store before the
         * Welford accumulators are zeroed.
         *
         * @param cls     Event-class index to reset.
         * @param newrun  When @c true, snapshot the run mean before clearing.
         */
        inline void reset(int cls, bool newrun) override
        {
            if(newrun && n.at(cls) > 0)
                s_runs.at(cls).push_back(a.at(cls));
            v.at(cls) = 0;
            n.at(cls) = 0;
            a.at(cls) = 0.0;
            q.at(cls) = 0.0;
        }
        /**
         * @brief End the current run for all classes and optionally start a new one.
         *
         * Increments the internal run counter when @p newrun is @c true, then
         * delegates to reset(int, bool) for each class.
         *
         * @param newrun  When @c true, snapshot each class mean and increment run count.
         */
        inline void reset(bool newrun) override
        {
            if(newrun)
                ++run;
            for(unsigned int i = 0; i < v.size(); i++)
                reset(i, newrun);
        }
        /**
         * @brief Full reset: discard all samples, run means, and run count.
         *
         * Unlike reset(), this also clears the inter-run store (@c s_runs) so
         * the object returns to its post-construction state.
         */
        inline void clear() override
        {
            for(unsigned int i = 0; i < v.size(); i++)
            {
                v.at(i) = 0;
                n.at(i) = 0;
                a.at(i) = 0.0;
                q.at(i) = 0.0;
            }
            s_runs = vector<vector<double>>(v.size(), vector<double>());
            run = 0;
        }
        /**
         * @brief Current-run mean for class @p cls (Welford running mean).
         *
         * Updated after every call to update(); valid even before the run ends.
         *
         * @param cls  Event-class index.
         * @return     Running mean of all samples collected in the current run.
         */
        inline double get(int cls)
        {
            return a.at(cls);
        }
        /** @copydoc get(int) */
        inline double mean(int cls)
        {
            return a.at(cls);
        }
        /**
         * @brief Grand mean: average of the per-run means stored so far.
         *
         * Only counts completed runs whose mean was snapshotted by reset(true).
         * Returns 0.0 if no runs have been completed yet.
         *
         * @param cls  Event-class index.
         * @return     Mean of the completed-run means, or 0.0 if none exist.
         */
        inline double get_scalar(int cls)
        {
            if(s_runs.at(cls).empty())
                return 0.0;
            double sum = 0;
            for(double val: s_runs.at(cls))
                sum += val;
            return sum / static_cast<double>(s_runs.at(cls).size());
        }
        /** @brief Number of completed runs whose mean has been snapshotted for class @p cls. */
        inline size_t completed_runs(int cls) const { return s_runs.at(cls).size(); }
        /** @brief Number of samples collected in the *current* (ongoing) run for class @p cls. */
        inline int n_updates(int cls) const { return n.at(cls); }
        /**
         * @brief Alias for get(int). Returns the current-run mean for class @p cls.
         * @deprecated Prefer get() or mean() for clarity.
         */
        inline double run_avg(int cls)
        {
            return get(cls);
        }
        /**
         * @brief Serialise current-run means for all classes as a tab-separated string.
         *
         * Each class contributes one tab-separated value (mean = sum / count),
         * followed by a newline.  Intended for logging and debugging.
         *
         * @return Tab-separated string of per-class current-run means.
         */
        inline string to_string() const override{
            string str = "";
            for(unsigned int i = 0; i < v.size(); i++)
            {
                double m = n.at(i) > 0 ? v.at(i) / static_cast<double>(n.at(i)) : 0.0;
                str += std::to_string(m) + "\t";
            }
            str += "\n";
            return str;
        }
        /**
         * @brief Student-t confidence interval for the mean of class @p cls.
         *
         * Built from the vector of per-run means collected via reset(true).
         * Requires at least two completed runs; returns [DBL_MIN, DBL_MAX]
         * if that condition is not met.
         *
         * @param alpha  Significance level (e.g. 0.05 for a 95 % CI).
         * @param cls    Event-class index.
         * @return       Pair (lower bound, upper bound) of the CI.
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

        /**
         * @brief Student-t confidence intervals for all classes.
         *
         * Convenience overload that calls confidence_interval(double, int) for
         * every class and returns the results in class-index order.
         *
         * @param alpha  Significance level (e.g. 0.05 for a 95 % CI).
         * @return       Vector of (lower, upper) pairs, one per class.
         */
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
        vector<double> v;               ///< Sum of all samples in the current run, per class.
        vector<int>    n;               ///< Sample count in the current run, per class.
        vector<double> a;               ///< Welford running mean (equals current-run mean), per class.
        vector<double> q;               ///< Welford M2 accumulator (sum of squared deviations), per class.
        unsigned int   run;             ///< Number of times reset(true) has been called.
        vector<vector<double>> s_runs;  ///< Per-class store of completed-run means.

        /**
         * @brief Feed one sample into the Welford online mean/variance accumulators.
         *
         * Updates v (sum), n (count), a (running mean), and q (M2) in O(1)
         * without loss of numerical precision.
         *
         * @param value  Observed sample value.
         * @param cls    Event-class index.
         */
        inline void welford_update(double value, int cls)
        {
            v.at(cls) += value;
            n.at(cls) += 1;
            double prev_a = a.at(cls);
            a.at(cls) += (value - a.at(cls)) / static_cast<double>(n.at(cls));
            q.at(cls) += (value - prev_a) * (value - a.at(cls));
        }
};

#endif