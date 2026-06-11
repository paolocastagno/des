#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>

#include "libdes_observer.hpp"
#include "libdes_observable.hpp"
#include "libdes_message.hpp"
#include "libdes_util.hpp"

using namespace std;

namespace des
{
	class histogram;
}

/**
 * @brief Observer that groups numeric samples into fixed-width bins.
 *
 * A histogram keeps one current-run bucket vector per event class. Incoming
 * values are quantized to @c floor(value / bin_size) * bin_size; summary
 * statistics are therefore computed over the binned values, not the raw input
 * values. Calling reset(true), reset(cls, true), or end_run() snapshots the
 * current buckets into @c s_runs so confidence intervals can be computed across
 * completed replications.
 */
class des::histogram : public des::observer {
    public:
        using observer::update;

        typedef struct bin
        {
            /**
             * @brief Binned value used as the bucket key.
             *
             * Values are produced by @c floor(sample / bin_size) * bin_size,
             * so this is the lower edge of the bucket for positive samples.
             */
            double value;
            /**
             * @brief Number of samples assigned to this bucket.
             */
            unsigned int count;
            /**
             * @brief Construct a new bin object
             * 
             */
            bin() : value(0), count(0) {}
            /**
             * @brief Construct a bin for bucket value @p v with zero count.
             *
             * @param v Binned value used as the bucket key.
             */
            bin(double v) : value(v), count(0)
            {}
            /**
             * @brief Construct a bin for bucket value @p v with count @p c.
             *
             * @param v Binned value used as the bucket key.
             * @param c Initial sample count for the bucket.
             */
            bin(double v, int c) : value(v), count(c)
            {}
            /**
             * @brief Copy-construct a bin from another bin.
             *
             * @param b Source bin.
             */
            bin(const bin& b)
            {
                value = b.value;
                count = b.count;
            }
            /**
             * @brief Copy another bin's bucket value and count into this bin.
             *
             * @param other Source bin.
             * @return Reference to this bin.
             */
            bin& operator=(const bin& other)
            {
                // Guard self assignment
                if (this == &other)
                    return *this;
            
                // assume *this manages a reusable resource, such as a heap-allocated buffer mArray
                else
                {
                    this->value=other.value;
                    this->count=other.count;
                } 
                return *this;
            }
            /**
             * @brief Compare bins by bucket value.
             *
             * @param b Bin to compare against.
             * @return @c true when both bins represent the same bucket value.
             */
            inline bool operator==(bin b)
            {
                return value == b.value;
            }
            /**
             * @brief Increase the bucket sample count.
             *
             * @return Reference to this bin after incrementing.
             */
            inline bin& operator++()
            {
                ++count;
                return *this;
            }
            /**
             * @brief Increase the bucket sample count.
             *
             * @return Copy of the bin before incrementing.
             */
            inline bin operator++(int)
            {
                bin old = *this; // copy old value
                operator++();  // prefix increment
                return old;
            }
            /**
             * @brief Decrease the bucket sample count.
             *
             * @return Reference to this bin after decrementing.
             */
            inline bin& operator--()
            {
                --count;
                return *this;
            }
            /**
             * @brief Order bins by bucket value.
             *
             * @param b Bin to compare against.
             * @return @c true when this bucket value is greater than @p b.
             */
            inline bool operator> (const bin& b) const 
            {
                if(value > b.value)
                {
                    return true;
                }
                return false;
            }
            /**
             * @brief Order bins by bucket value.
             *
             * @param b Bin to compare against.
             * @return @c true when this bucket value is less than @p b.
             */
            inline bool operator< (const bin& b) const
            {
                if(value < b.value)
                {
                    return true;
                }
                return false;
            }
        } bin;

        /**
         * @brief Construct a histogram observer.
         *
         * @param description String identifier of the observer and incoming
         *                    message key to read.
         * @param cls Number of event classes to track independently.
         */
        histogram(const string& description, int cls) : observer() // description, id_gen++){}
        {
            observer_id = description;
            bin_size = 1e-4;
            const auto class_count = static_cast<size_t>(cls);
            for(size_t i = 0; i < class_count; i++)
            {
                v.push_back({});
                elements.push_back(0);
                sum.push_back(0);
            }
            run = 0;
            s_runs = vector<vector<vector<histogram::bin>>>(class_count, vector<vector<histogram::bin>>());
        }
        /**
         * @brief Construct a histogram observer already associated with an event.
         *
         * @param description String identifier of the observer and incoming
         *                    message key to read.
         * @param evnt Observable event string identifier.
         * @param cls Number of event classes to track independently.
         */
        histogram(const string& description, const string& evnt, int cls) : histogram(description, cls)
        {
            event = evnt;
        }
        /**
         * @brief Add one sample to the histogram for class @p cls.
         *
         * The raw sample is quantized to the configured bucket size before it
         * is counted. Running mean and standard deviation use that binned value.
         *
         * @param value Raw observed value.
         * @param cls Event-class index.
         */
        inline void update(double value, int cls){
            int rounded_value = static_cast<int>(floor(value/bin_size));
            const double binned_value = static_cast<double>(rounded_value)*bin_size;
            vector<histogram::bin>::iterator fnd = std::find(v.at(cls).begin(), v.at(cls).end(), binned_value);
            if(fnd != v.at(cls).end())
            {
                ++(*fnd);
            }
            else
            {
                int current = v.at(cls).size();
                while(current <= rounded_value)
                {
                    v.at(cls).push_back(histogram::bin(static_cast<double>(current)*bin_size, 0));
                    ++current;
                }
                ++(v.at(cls).at(current-1));
            }
            sum.at(cls) += binned_value;
            ++(elements.at(cls));
        }
        /**
         * @brief Update from a serialized observer message.
         *
         * Reads the sample value from the key named by @c observer_id and the
         * class index from @c EVENT_CLS.
         *
         * @param m Serialized @c des::message.
         */
        inline void update(string m) override{
            message msg(m);
            update(msg.get_value(observer_id), msg.get_value(EVENT_CLS));
        }
        /**
         * @brief Set the bin width while no current-run buckets exist.
         *
         * @param bs New bin width.
         * @return @c true if every current-run histogram is empty and the bin
         *         size was changed; @c false otherwise.
         */
        bool set_binsize(double bs)
        {
            bool all_empty = true;
            for(unsigned int j = 0; j < v.size(); j++)
            {
                if(v.at(j).size() != 0)
                    all_empty = false;
            }
            if(all_empty)
                bin_size = bs;
            return all_empty;
        }
        /**
         * @brief Current-run mean of binned values for class @p cls.
         *
         * @param cls Event-class index.
         * @return Mean binned value for the current run.
         */
        inline double mean(int cls)
        {
            return sum.at(cls) / static_cast<double>(elements.at(cls));
        }
        /**
         * @brief Number of samples observed in the current run for class @p cls.
         *
         * @param cls Event-class index.
         * @return Current-run sample count.
         */
        inline unsigned int observations(int cls)
        {
            return elements.at(cls);
        }
        /**
         * @brief Population standard deviation of current-run binned values.
         *
         * Returns @c __DBL_MAX__ when fewer than two samples have been observed.
         *
         * @param cls Event-class index.
         * @return Population standard deviation of binned values.
         */
        inline double stddev(int cls) const
        {
            double sdev = __DBL_MAX__;
            if(elements.at(cls) >= 2)
            {
                sdev = 0;
                double m = sum.at(cls) / static_cast<double>(elements.at(cls));
                for(unsigned int j = 0; j < v.at(cls).size(); j++)
                {
                    sdev += pow(m-v.at(cls).at(j).value,2)*v.at(cls).at(j).count;
                }
                sdev = sqrt(sdev / static_cast<double>(elements.at(cls)));
            }
            return sdev;
        }
        /**
         * @brief Reset one class, optionally storing it as a completed run.
         *
         * @param cls Event-class index to reset.
         * @param newrun When @c true, store the current class histogram in the
         *               completed-run store before clearing it.
         */
        inline void reset(int cls, bool newrun) override
        {
            if(newrun)
            {
                store_current_class(cls);
                return;
            }
            reset_current_class(cls);
        }
        /**
         * @brief Reset all classes, optionally storing them as a completed run.
         *
         * @param newrun When @c true, store every current class histogram in the
         *               completed-run store before clearing them.
         */
        inline void reset(bool newrun) override
        {
            if(newrun)
            {
                end_run();
                return;
            }
            reset_current();
        }
        /**
         * @brief Reset all classes, optionally storing rate-normalized buckets.
         *
         * When @p newrun is @c true, bucket values are divided by @p time before
         * the histograms are stored as a completed run.
         *
         * @param time Sampling interval used to normalize bucket values.
         * @param newrun When @c true, store a normalized completed run.
         */
        inline void reset(double time, bool newrun)
        {
            if(newrun)
            {
                end_run(time);
                return;
            }
            reset_current();
        }
        /**
         * @brief Clear current-run buckets and counters without storing a run.
         */
        inline void clear() override
        {
            reset_current();
        }
        /**
         * @brief Current-run histograms for all classes.
         *
         * @return Vector indexed by class, then by bucket.
         */
        inline vector<vector<bin>> get()
        {
            return v;
        }
        /**
         * @brief Format current-run summary statistics for logging.
         *
         * @return String containing the observer id and per-class summaries.
         */
        inline string to_string() const override{
            string str = "\t" + observer_id;
            for(unsigned int i = 0; i < v.size(); i++)
            {
                str = "\t(" + std::to_string(i) + ")\t" + std::to_string(sum.at(i) / static_cast<double>(elements.at(i))) + "(" + std::to_string(stddev(i)*0.5) + ")";
            }
            return str;
        }
        /**
         * @brief Store all current histograms as one completed run and clear them.
         */
        inline void end_run()
        {
            for(unsigned int cls = 0; cls < v.size(); cls++)
            {
                store_current_class(cls);
            }
            ++run;
        }
        /**
         * @brief Store all current histograms after normalizing bucket values.
         *
         * @param time Sampling interval used to convert bucket values into rates.
         */
        inline void end_run(double time)
        {
            for(unsigned int cls = 0; cls < v.size(); cls++)
            {
                for(unsigned int i = 0; i < v.at(cls).size(); i++)
                {
                    v.at(cls).at(i).value /= time;
                }
                store_current_class(cls);
            }
            ++run;
        }

        /**
         * @brief Confidence interval per bucket for a completed-run class.
         *
         * Each row contains bucket value, mean count, lower bound, and upper
         * bound across completed runs.
         *
         * @param alpha Significance level.
         * @param cls Event-class index.
         * @return Rows of @c {bucket, mean, lower, upper}; empty if fewer than
         *         two completed runs are available.
         */
        inline vector<vector<double>> confidence_interval(double alpha, int cls)
        {
            if(s_runs.size() > 0 && s_runs.at(cls).size() > 1)
            {
                vector<vector<double>> ci;
                vector<double> obs(s_runs.at(cls).size(), 0);
                for(size_t i = 0; i < s_runs.at(cls).at(0).size(); i++)
                {
                    double mean = 0;
                    pair<double,double> interval;
                    for(size_t j = 0; j < s_runs.at(cls).size(); j++)
                    {
                        obs.at(j) = s_runs.at(cls).at(j).at(i).count;
                        mean += s_runs.at(cls).at(j).at(i).count;
                    }
                    mean /= static_cast<double>(s_runs.at(cls).size());
                    interval = conf_int(obs, mean, alpha);
                    ci.push_back({s_runs.at(cls).at(0).at(i).value, mean, interval.first, interval.second});
                }
                return ci;
            }
            else
            {
                return vector<vector<double>>();
            }
        }

        /**
         * @brief Confidence intervals per bucket for every class.
         *
         * @param alpha Significance level.
         * @return Outer vector indexed by class.
         */
        inline vector<vector<vector<double>>> confidence_interval(double alpha)
        {
            vector<vector<vector<double>>> ci;
            if(s_runs.size() > 0)
            {
                for(size_t i = 0; i < s_runs.size(); i++)
                {
                    ci.push_back(confidence_interval(alpha, static_cast<int>(i)));
                }
            }
            return ci;
        }

        /**
         * @brief Render histogram data as CSV-style rows.
         *
         * If completed runs exist, prints bucket confidence-interval rows.
         * Otherwise prints current-run bucket values and counts.
         *
         * @param alpha Significance level used for confidence intervals.
         * @return CSV-style text representation.
         */
        inline string print(double alpha = 1e-2)
        {
            std::stringstream stream;
            if(s_runs.size() > 0)
            {
                vector<vector<vector<double>>> ci = confidence_interval(alpha);
                for(unsigned int i = 0; i < ci.size(); ++i)
                {
                    for(unsigned int j = 0; j < ci.at(i).size(); j++)
                    {
                        for(unsigned int k = 0; k < ci.at(i).at(j).size(); k++)
                        {
                            if(k != 0)
                            {
                                stream << ",";
                            }
                            stream << std::fixed << std::setprecision(9) << ci.at(i).at(j).at(k);
                        }
                        stream << "\n";
                    }
                    stream << "\n\n";
                }
            }
            else
            {
                for(unsigned int i = 0; i < v.size(); ++i)
                {
                    for(bin& b: v.at(i))
                    {
                        stream << std::fixed << std::setprecision(9) << b.value-bin_size << ",";
                        stream << std::fixed << std::setprecision(9) << b.count << "\n";
                    }
                    stream << "\n";
                }
            }
            return stream.str();
        }

    private:
        /**
         * @brief Clear the current-run state for one class.
         *
         * Completed-run snapshots in @c s_runs are preserved.
         */
        inline void reset_current_class(int cls)
        {
            sum.at(cls) = 0;
            elements.at(cls) = 0;
            v.at(cls).clear();
        }

        /**
         * @brief Clear current-run state for all classes.
         *
         * Completed-run snapshots in @c s_runs are preserved.
         */
        inline void reset_current()
        {
            for(unsigned int i = 0; i < v.size(); i++)
            {
                reset_current_class(i);
            }
        }

        /**
         * @brief Snapshot one class into the completed-run store and clear it.
         *
         * Existing runs for the class are first aligned so every run has a row
         * for the same bucket values.
         */
        inline void store_current_class(int cls)
        {
            align_run_bins(cls);
            s_runs.at(cls).push_back(v.at(cls));
            reset_current_class(cls);
        }

        /**
         * @brief Add zero-count buckets to @p target for buckets in @p reference.
         *
         * This keeps completed runs comparable even when a bucket appears in one
         * replication but not another.
         */
        inline void add_missing_bins(vector<histogram::bin>& target, const vector<histogram::bin>& reference)
        {
            vector<histogram::bin> missing_bins;
            for(const histogram::bin& bin: reference)
            {
                if(std::find(target.begin(), target.end(), bin) == target.end())
                {
                    missing_bins.push_back(histogram::bin(bin.value, 0));
                }
            }
            if(missing_bins.empty())
            {
                return;
            }

            vector<histogram::bin> merged;
            merge(target.begin(), target.end(), missing_bins.begin(), missing_bins.end(), std::back_inserter(merged));
            target = merged;
        }

        /**
         * @brief Align current and completed histograms for one class by bucket value.
         */
        inline void align_run_bins(int cls)
        {
            vector<histogram::bin> reference = v.at(cls);
            for(unsigned int i = 0; i < s_runs.at(cls).size(); i++)
            {
                add_missing_bins(reference, s_runs.at(cls).at(i));
            }
            for(unsigned int i = 0; i < s_runs.at(cls).size(); i++)
            {
                add_missing_bins(s_runs.at(cls).at(i), reference);
            }
            v.at(cls) = reference;
        }

        vector<vector<histogram::bin>> v;              ///< Current-run buckets, indexed by class.
        double bin_size;                                         ///< Width used to quantize raw sample values.
        vector<unsigned int> elements;                      ///< Current-run sample count, per class.
        vector<double> sum;                                 ///< Sum of current-run binned values, per class.
        int run;                                                 ///< Number of completed all-class runs.
        vector<vector<vector<histogram::bin>>> s_runs; ///< Completed-run buckets, indexed by class then run.
};

#endif
