#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <random>
#include <cmath>
#include <cstddef>
#include <utility>

#include "incbeta.hpp"

/**
 * @brief Compute the population standard deviation of a numeric sample.
 *
 * This helper preserves the library's existing population-standard-deviation
 * behavior by dividing by @c n. Confidence intervals use sample_stddev()
 * instead, because their standard error must be based on the Bessel-corrected
 * sample variance.
 *
 * @param v Numeric observations.
 * @param mean Mean of @p v, supplied by the caller to avoid recomputing it.
 * @return Population standard deviation, or @c __DBL_MAX__ when @p v has
 * fewer than two observations.
 */
template <typename T>
inline double stddev(const std::vector<T>& v, double mean)
{
    double sdev = __DBL_MAX__;
    size_t size = v.size();
    if(size >= 2)
    {
        sdev = 0;
        for(size_t j = 0; j < size; j++)
        {
            sdev += pow(mean-v.at(j),2);
        }
        sdev = sqrt(sdev / (static_cast<double>(size)));
    }
    return sdev;
} 

/**
 * @brief Compute the sample standard deviation of a numeric sample.
 *
 * Divides by @c n - 1 and is therefore the appropriate dispersion estimate
 * for Student-t confidence intervals over independent replications.
 *
 * @param v Numeric observations.
 * @param mean Mean of @p v, supplied by the caller to avoid recomputing it.
 * @return Sample standard deviation, or @c __DBL_MAX__ when @p v has fewer
 * than two observations.
 */
template <typename T>
inline double sample_stddev(const std::vector<T>& v, double mean)
{
    double sdev = __DBL_MAX__;
    size_t size = v.size();
    if(size >= 2)
    {
        sdev = 0;
        for(size_t j = 0; j < size; j++)
        {
            sdev += pow(mean-v.at(j),2);
        }
        sdev = sqrt(sdev / (static_cast<double>(size - 1)));
    }
    return sdev;
}

/**
 * @brief Compute the arithmetic mean of a non-empty numeric vector.
 *
 * @param v Numeric observations. The caller must ensure @p v is not empty.
 * @return Arithmetic mean of @p v.
 */
template <typename T>
inline double vector_mean(const std::vector<T>& v)
{
    double mean = 0;
    for(const T& d: v)
        mean += static_cast<double>(d);
    return mean / static_cast<double>(v.size());
}

/**
 * @brief Student-t cumulative distribution function.
 *
 * Uses the regularized incomplete beta implementation provided by incbeta.hpp.
 *
 * @param t Student-t variate.
 * @param degrees_freedom Distribution degrees of freedom.
 * @return P(T <= @p t).
 */
inline double student_t_cdf(double t, double degrees_freedom) {
    /*The cumulative distribution function (CDF) for Student's t distribution*/
    double x = (t + sqrt(t * t + degrees_freedom)) / (2.0 * sqrt(t * t + degrees_freedom));
    double prob = incbeta(degrees_freedom/2.0, degrees_freedom/2.0, x);
    return prob;
}

/**
 * @brief Inverse Student-t CDF.
 *
 * This returns the quantile @c t such that @c student_t_cdf(t, df) is
 * approximately @p probability. It exploits symmetry for lower-tail
 * probabilities, expands the positive search bracket, then refines the result
 * with binary search.
 *
 * @param probability Target cumulative probability in [0, 1].
 * @param degrees_freedom Distribution degrees of freedom.
 * @return Student-t quantile for @p probability and @p degrees_freedom.
 */
inline double student_t_quantile(double probability, double degrees_freedom)
{
    if(probability <= 0.0)
        return -__DBL_MAX__;
    if(probability >= 1.0)
        return __DBL_MAX__;
    if(degrees_freedom <= 0.0)
        return __DBL_MAX__;
    if(probability < 0.5)
        return -student_t_quantile(1.0 - probability, degrees_freedom);

    // Grow the upper bound until it contains the requested upper-tail quantile.
    double low = 0.0;
    double high = 1.0;
    while(student_t_cdf(high, degrees_freedom) < probability && high < 1.0e10)
    {
        low = high;
        high *= 2.0;
    }

    // The Student-t CDF is monotonic, so binary search is stable here.
    for(int i = 0; i < 100; i++)
    {
        double mid = (low + high) / 2.0;
        if(student_t_cdf(mid, degrees_freedom) < probability)
            low = mid;
        else
            high = mid;
    }
    return (low + high) / 2.0;
}

/**
 * @brief Student-t confidence interval for a mean with a supplied sample mean.
 *
 * The interval is computed as @c mean +/- t_(1 - alpha/2, n - 1) * s / sqrt(n),
 * where @c s is the Bessel-corrected sample standard deviation.
 *
 * @param v Independent observations, usually one value per completed run.
 * @param mean Mean of @p v, supplied by the caller.
 * @param alpha Significance level, for example 0.05 for a 95 percent CI.
 * @return Lower and upper confidence bounds, or @c [__DBL_MIN__, __DBL_MAX__]
 * when the interval cannot be computed.
 */
template <typename T>
inline std::pair<double, double> conf_int(const std::vector<T>& v, double mean, double alpha)
{
    if(v.size() < 2 || alpha <= 0.0 || alpha >= 1.0)
        return std::pair<double,double>(__DBL_MIN__, __DBL_MAX__);
    double sdev = sample_stddev(v, mean);
    double critical_value = student_t_quantile(1.0 - alpha / 2.0, v.size()-1);
    double interval = sdev/sqrt(v.size()) * critical_value;
    return std::pair<double,double>(mean-interval, mean+interval);
}

/**
 * @brief Student-t confidence interval for a mean.
 *
 * Computes the mean of @p v and delegates to the overload that accepts an
 * explicit mean, keeping all confidence-interval math in one implementation.
 *
 * @param v Independent observations, usually one value per completed run.
 * @param alpha Significance level, for example 0.05 for a 95 percent CI.
 * @return Lower and upper confidence bounds, or @c [__DBL_MIN__, __DBL_MAX__]
 * when the interval cannot be computed.
 */
template <typename T>
inline std::pair<double, double> conf_int(const std::vector<T>& v, double alpha)
{
    if(v.size() < 2 || alpha <= 0.0 || alpha >= 1.0)
        return std::pair<double,double>(__DBL_MIN__, __DBL_MAX__);
    double mean = vector_mean(v);
    return conf_int(v, mean, alpha);
}

#endif
