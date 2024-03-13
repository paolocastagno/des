#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <random>
#include <cmath>

#include "incbeta.hpp"
       
/**
 * @brief Computes the Standard deviation for a class
 * 
 * @param v vector of observations
 * @param mean the mean value of the observation 
 * 
 * @return double 
 */
inline double stddev(std::vector<double> v, double mean)
{
    double sdev = __DBL_MAX__;
    int size = v.size();
    if(size >= 2)
    {
        sdev = 0;
        for(unsigned int j = 0; j < size; j++)
        {
            sdev += pow(mean-v.at(j),2);
        }
        sdev = sqrt(sdev / (static_cast<double>(size)));
    }
    return sdev;
} 

inline double stddev(std::vector<int> v, double mean)
{
    double sdev = __DBL_MAX__;
    int size = v.size();
    if(size >= 2)
    {
        sdev = 0;
        for(unsigned int j = 0; j < size; j++)
        {
            sdev += pow(mean-v.at(j),2);
        }
        sdev = sqrt(sdev / (static_cast<double>(size)));
    }
    return sdev;
} 

inline double student_t_cdf(double t, double v) {
    /*The cumulative distribution function (CDF) for Student's t distribution*/
    double x = (t + sqrt(t * t + v)) / (2.0 * sqrt(t * t + v));
    double prob = incbeta(v/2.0, v/2.0, x);
    return prob;
}

inline std::pair<double, double> conf_int(std::vector<double> v, double mean, double alpha)
{
    double sdev = stddev(v, mean);
    double interval = sdev/sqrt(v.size()) * student_t_cdf(v.size()-1, alpha);
    return std::pair<double,double>(mean-interval, mean+interval);
}

inline std::pair<double, double> conf_int(std::vector<double> v, double alpha)
{
    double mean = 0;
    for(double d: v)
        mean += d;
    mean = mean / static_cast<double>(v.size());
    return conf_int(v, mean, alpha);
}

inline std::pair<double, double> conf_int(std::vector<int> v, double mean, double alpha)
{
    double sdev = stddev(v, mean);
    double interval = sdev/sqrt(v.size()) * student_t_cdf(v.size()-1, alpha);
    return std::pair<double,double>(mean-interval, mean+interval);
}

inline std::pair<double, double> conf_int(std::vector<int> v, double alpha)
{
    double mean = 0;
    for(int d: v)
        mean += static_cast<double>(d);
    mean = mean / static_cast<double>(v.size());
    return conf_int(v, mean, alpha);
}

#endif