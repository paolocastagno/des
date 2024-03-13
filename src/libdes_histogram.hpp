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

class des::histogram : public des::observer {
    public:
        typedef struct bin
        {
            /**
             * @brief Bin's upper bound
             * 
             */
            double value;
            /**
             * @brief Number of objects belonging to the bin
             * 
             */
            unsigned int count;
            /**
             * @brief Construct a new bin object
             * 
             */
            bin() : value(0), count(0) {}
            /**
             * @brief Construct a new bin object
             * 
             * @param v 
             */
            bin(double v) : value(v), count(0)
            {}
            /**
             * @brief Construct a new bin object
             * 
             * @param v 
             * @param c 
             */
            bin(double v, int c) : value(v), count(c)
            {}
            /**
             * @brief Construct a new bin object
             * 
             * @param b 
             */
            bin(const bin& b)
            {
                value = b.value;
                count = b.count;
            }
            /**
             * @brief Overload of copy assignment
             * 
             * @param other 
             * @return bin 
             */
            bin& operator=(bin& other)
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
             * @brief Define the equals operator
             * 
             * @param lhs 
             * @param rhs 
             * @return true 
             * @return false 
             */
            inline bool operator==(bin b)
            {
                return value == b.value;
            }
            /**
             * @brief Define prefix increment operator
             * 
             * @return bin& 
             */
            inline bin& operator++()
            {
                ++count;
                return *this;
            }
            /**
             * @brief Define postfix increment operator
             * 
             * @return bin 
             */
            inline bin operator++(int)
            {
                bin old = *this; // copy old value
                operator++();  // prefix increment
                return old;
            }
            /**
             * @brief Define prefix decrement operator
             * 
             * @return bin& 
             */
            inline bin& operator--()
            {
                --count;
                return *this;
            }
            /**
             * @brief Define greater then operator
             * 
             * @param b 
             * @return int 
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
             * @brief Define less than operator
             * 
             * @param b 
             * @return true 
             * @return false 
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
         * @brief Construct a new histogram object
         * 
         * @param description String identifier of the observer
         */
        histogram(const string& description, int cls) : observer() // description, id_gen++){}
        {
            observer_id = description;
            bin_size = 1e-4;
            for(unsigned int i = 0; i < cls; i++)
            {
                v.push_back({});
                elements.push_back(0);
                sum.push_back(0);
            }
            run = 0;
            s_runs = std::vector<std::vector<std::vector<histogram::bin>>>(cls, std::vector<std::vector<histogram::bin>>());
        }
        /**
         * @brief Construct a new histogram object
         * 
         * @param description observer's string identifier
         * @param evnt observable's event string identifier 
         */
        histogram(const string& description, const string& evnt, int cls) : histogram(description, cls)
        {
            event = evnt;
        }
        /**
         * @brief Updates the histogram value
         * 
         */
        inline void update(double value, int cls){
            int rounded_value = static_cast<int>(floor(value/bin_size));
            std::vector<histogram::bin>::iterator fnd = std::find(v.at(cls).begin(), v.at(cls).end(), rounded_value*bin_size);
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
            sum.at(cls) += rounded_value;
            ++(elements.at(cls));
        }
        /**
         * @brief Updates the histogram value
         * 
         */
        inline void update(string m) override{
            message msg(m);
            update(msg.get_value(observer_id), msg.get_value(EVENT_CLS));
        }
        /**
         * @brief Set the binsize
         * 
         * @param bs 
         * @return true if all the histograms are empty, and the binsisze has been changed 
         * @return false otherwise
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
         * @brief Computes the mean for a class
         * 
         * @param cls 
         * @return double 
         */
        inline double mean(int cls)
        {
            return sum.at(cls) / static_cast<double>(elements.at(cls));
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
                double mean = sum.at(cls) / static_cast<double>(elements.at(cls));
                for(unsigned int j = 0; j < v.at(cls).size(); j++)
                {
                    sdev += pow(mean-v.at(cls).at(j).value,2)*v.at(cls).at(j).count;
                }
                sdev = sqrt(sdev / static_cast<double>(elements.at(cls)));
            }
            return sdev;
        }
        /**
         * @brief Resets the histogram for the given class and value
         * 
         * @param value 
         * @param cls 
         */
        inline void reset(int cls, bool newrun) override
        {
            sum.at(cls) = 0;
            elements.at(cls);
            v.at(cls).clear();
        }
        /**
         * @brief 
         * 
         * @param value 
         */
        inline void reset(bool newrun) override
        {
            if(newrun)
            {
                end_run();
            }
            for(int i = 0; i < v.size(); i++)
            {
                reset(i, newrun);
            }
        }
        /**
         * @brief 
         * 
         * @param value 
         */
        inline void reset(double time, bool newrun)
        {
            if(newrun)
            {
                end_run(time);
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
                sum.at(i) = 0;
                elements.at(i);
                v.at(i).clear();
            }
        }
        /**
         * @brief returns the current value of the histogram
         * 
         * @return double 
         */
        inline std::vector<std::vector<bin>> get()
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
                str = "\t(" + std::to_string(i) + ")\t" + std::to_string(sum.at(i) / static_cast<double>(elements.at(i))) + "(" + std::to_string(stddev(i)*0.5) + ")";
            }
            return str;
        }
        /**
         * @brief Store the current value of the scalar and reset its state
         *  
         */
        inline void end_run()
        {
            // Make all the stored histograms of the same lenght
            if(run > 0)
            {
                for(int cls = 0; cls < v.size(); cls++)
                {
                    //Find the max value stored in the previous runs
                    int s_max = s_runs.at(cls).at(0).size();
                    //Find the current max
                    int max = v.at(cls).size();
                    if(max > s_max)
                    {
                        // Add further elements in the stored histograms
                        int missing = max-s_max;
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        int base = s_runs.at(cls).at(0).size();
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = v.at(cls).at(base + i).value;
                        }
                        for(int i = 0; i < run; i++)
                        {
                            std::vector<histogram::bin> tmp;
                            merge(s_runs.at(cls).at(i).begin(), s_runs.at(cls).at(i).end(), missing_bin.begin(), missing_bin.end(), std::back_inserter(tmp));
                            s_runs.at(cls).at(i) = tmp;
                        }
                    }
                    else
                    {
                        // Add further elements in the current histogram
                        int missing = s_max-max;
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        int base = v.at(cls).size();
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = s_runs.at(0).at(cls).at(base + i).value;
                        }
                        std::vector<histogram::bin> tmp;
                        merge(v.at(cls).begin(), v.at(cls).end(), missing_bin.begin(), missing_bin.end(), std::back_inserter(tmp));
                        v.at(cls) = tmp;
                    }
                }
            }
            // Store the histograms appropriately
            for(int i = 0; i < v.size(); i++)
            {
                s_runs.at(i).push_back(v.at(i));
            }
            ++run;
        }
        /**
         * @brief Store the current value of the scalar (as a rate) and reset its state
         * 
         * @param time sampling time interval duration
         */
        inline void end_run(double time)
        {
            // Make all the stored histograms of the same lenght
            if(run > 0)
            {
                for(int cls = 0; cls < v.size(); cls++)
                {
                    //Find min and max valued stored in the previous runs
                    double s_min = s_runs.at(cls).at(0).at(0).value;
                    double s_max = s_runs.at(cls).at(0).at(s_runs.at(cls).at(0).size()-1).value;
                    //Find the current min and max
                    double min = v.at(cls).at(0).value;
                    double max = v.at(cls).at(v.at(cls).size()-1).value;
                    if(min < s_min)
                    {
                        // Add further elements in the stored histograms
                        int missing = static_cast<int>(ceil((s_min-min)/bin_size));
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = v.at(cls).at(i).value;
                        }
                        for(int i = 0; i < run; i++)
                        {
                            std::vector<histogram::bin> tmp = s_runs.at(cls).at(i);
                            merge(missing_bin.begin(), missing_bin.end(), tmp.begin(), tmp.end(), std::back_inserter(s_runs.at(i).at(cls)));
                        }
                    }
                    else if(min > s_min)
                    {
                        // Add further elements in the current histogram
                        int missing = static_cast<int>(ceil((min-s_min)/bin_size));
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = s_runs.at(0).at(cls).at(i).value;
                        }
                        std::vector<histogram::bin> tmp = v.at(cls);
                        merge(missing_bin.begin(), missing_bin.end(), tmp.begin(), tmp.end(), std::back_inserter(v.at(cls)));
                    }
                    else if(max > s_max)
                    {
                        // Add further elements in the stored histograms
                        int missing = static_cast<int>(ceil((max-s_max)/bin_size));
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        int base = s_runs.at(cls).at(0).size();
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = v.at(cls).at(base + i).value;
                        }
                        for(int i = 0; i < run; i++)
                        {
                            std::vector<histogram::bin> tmp = s_runs.at(i).at(cls);
                            merge(tmp.begin(), tmp.end(),missing_bin.begin(), missing_bin.end(),  std::back_inserter(s_runs.at(i).at(cls)));
                        }
                    }
                    else if(max < s_max)
                    {
                        // Add further elements in the current histogram
                        int missing = static_cast<int>(ceil((s_min-min)/bin_size));
                        std::vector<histogram::bin> missing_bin(missing, histogram::bin(0,0));
                        int base = v.at(cls).size();
                        for(int i = 0; i < missing; i++)
                        {
                            missing_bin.at(i).value = s_runs.at(0).at(cls).at(base + i).value;
                        }
                        std::vector<histogram::bin> tmp = v.at(cls);
                        merge(tmp.begin(), tmp.end(), missing_bin.begin(), missing_bin.end(), std::back_inserter(v.at(cls)));
                    }
                }
            }
            // Store the histograms appropriately
            for(int i = 0; i < v.size(); i++)
            {
                // Divide the values
                for(int j = 0; j < v.at(i).size(); j++)
                {
                    v.at(i).at(j).value /= time;
                }
                s_runs.at(i).push_back(v.at(i));
            }
            ++run;
            reset(0);
        }

        inline std::vector<std::vector<double>> confidence_interval(double alpha, int cls)
        {
            if(s_runs.size() > 0 && s_runs.at(cls).size() > 1)
            {
                std::vector<std::vector<double>> ci;
                std::vector<double> obs(s_runs.at(cls).size(), 0);
                for(int i = 0; i < s_runs.at(cls).at(0).size(); i++)
                {
                    double mean = 0;
                    pair<double,double> interval;
                    for(int j = 0; j < s_runs.at(cls).size(); j++)
                    {
                        obs.at(j) = s_runs.at(cls).at(j).at(i).count;
                        mean += s_runs.at(cls).at(j).at(i).count;
                    }
                    mean /= s_runs.at(cls).size();
                    interval = conf_int(obs, mean, alpha);
                    ci.push_back({s_runs.at(cls).at(0).at(i).value, mean, interval.first, interval.second});
                }
                return ci;
            }
            else
            {
                return std::vector<std::vector<double>>();
            }
        }

        inline std::vector<std::vector<std::vector<double>>> confidence_interval(double alpha)
        {
            std::vector<std::vector<std::vector<double>>> ci;
            if(s_runs.size() > 0)
            {
                for(int i = 0; i < s_runs.size(); i++)
                {
                    ci.push_back(confidence_interval(alpha,i));
                }
            }
            return ci;
        }

        inline string print(double alpha = 1e-2)
        {
            std::stringstream stream;
            if(s_runs.size() > 0)
            {
                std::vector<std::vector<std::vector<double>>> ci = confidence_interval(alpha);
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
         * @brief measures' identifier generator
         * 
         */
        // inline static int id_gen = 0;
        /**
         * @brief the histogram
         * 
         */
        std::vector<std::vector<histogram::bin>> v;
        double bin_size;
        std::vector<unsigned int> elements;
        std::vector<double> sum;
        int run;
        std::vector<std::vector<std::vector<histogram::bin>>> s_runs;
};

#endif