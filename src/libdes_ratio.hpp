#ifndef RATIO_H
#define RATIO_H

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "libdes_observer.hpp"
#include "libdes_message.hpp"
#include "libdes_util.hpp"

using namespace std;

namespace des
{
	class ratio;
}

/**
 * @brief Ratio estimator Σx / Σy over independent pairs (x, y), per class, within a run
 *        and across runs.
 *
 * Its typical use is the regenerative method: the cycles between two regeneration points
 * of a model are independent and identically distributed, and each contributes one pair,
 * e.g. the jobs a station served in the cycle (x) and the cycle's length (y). The
 * throughput of the run is then r = Σx / Σy. Because x and y are correlated, the variance
 * of r depends on their covariance, so the ratio keeps the means and the co-moments of x
 * and y (Welford's online algorithm) and gives the run's confidence interval
 *
 *     r ± t(1 - alpha/2, k - 1) · s / (ȳ · √k),   s² = Σ(x − r·y)² / (k − 1),
 *
 * where k is the number of pairs and ȳ = Σy / k.
 *
 * Like des::scalar, reset(true) ends a run and stores its estimate r, and
 * confidence_interval() gives the Student-t interval of the mean of the stored estimates
 * across runs. A ratio reads no message field: it is fed with update(x, y, cls) only.
 */
class des::ratio : public des::observer
{
    public:
        /**
         * @brief Construct a ratio fed only with update(x, y, cls).
         *
         * @param description  Observer identifier.
         * @param cls          Number of event classes to track independently.
         */
        ratio(const string& description, int cls) : observer()
        {
            observer_id = description;
            const auto class_count = static_cast<size_t>(cls);
            cur = vector<moments>(class_count);
            s_runs = vector<vector<double>>(class_count, vector<double>());
        }
        /**
         * @brief A ratio reads no message field.
         *
         * @throws logic_error always: feed it with update(x, y, cls)
         */
        inline void update(const des::message&) override
        {
            throw logic_error("des::ratio " + observer_id + " reads no message field: feed it with update(x, y, cls)");
        }
        /**
         * @brief Add the pair (@p x, @p y) to the current run of class @p cls.
         *
         * @param x    Numerator term, e.g. the jobs served in a regeneration cycle.
         * @param y    Denominator term, e.g. the length of that cycle.
         * @param cls  Event-class index (0-based).
         */
        inline void update(double x, double y, int cls)
        {
            moments& m = cur.at(cls);
            m.k += 1;
            const double dx = x - m.mx;
            const double dy = y - m.my;
            m.mx += dx / static_cast<double>(m.k);
            m.my += dy / static_cast<double>(m.k);
            m.cxx += dx * (x - m.mx);
            m.cyy += dy * (y - m.my);
            m.cxy += dx * (y - m.my);
        }
        /**
         * @brief Current-run estimate Σx / Σy for class @p cls; 0.0 while Σy is 0.
         */
        inline double get(int cls) const
        {
            const moments& m = cur.at(cls);
            return m.my != 0.0 ? m.mx / m.my : 0.0;
        }
        /** @brief Number of pairs added in the *current* (ongoing) run for class @p cls. */
        inline long n_updates(int cls) const { return cur.at(cls).k; }
        /**
         * @brief Confidence interval of the current-run estimate for class @p cls.
         *
         * Assumes the pairs are independent and identically distributed, as regeneration
         * cycles are. Requires at least two pairs and Σy ≠ 0; returns
         * [-DBL_MAX, DBL_MAX] otherwise.
         *
         * @param alpha  Significance level (e.g. 0.05 for a 95 % CI).
         * @param cls    Event-class index.
         * @return       Pair (lower bound, upper bound) of the CI.
         */
        inline pair<double, double> run_confidence_interval(double alpha, int cls) const
        {
            const moments& m = cur.at(cls);
            if(m.k < 2 || m.my == 0.0 || alpha <= 0.0 || alpha >= 1.0)
            {
                return pair<double, double>(-__DBL_MAX__, __DBL_MAX__);
            }
            const double r = m.mx / m.my;
            const double k = static_cast<double>(m.k);
            // Σ(x − r·y)² from the co-moments: the mean of x − r·y is 0 by definition of r
            const double s2 = max(0.0, (m.cxx - 2.0 * r * m.cxy + r * r * m.cyy) / (k - 1.0));
            const double half = student_t_quantile(1.0 - alpha / 2.0, k - 1.0) * sqrt(s2) / (fabs(m.my) * sqrt(k));
            return pair<double, double>(r - half, r + half);
        }
        /**
         * @brief Mean of the estimates of the completed runs of class @p cls; 0.0 if none.
         */
        inline double get_ratio(int cls) const
        {
            return s_runs.at(cls).empty() ? 0.0 : vector_mean(s_runs.at(cls));
        }
        /** @brief Number of completed runs whose estimate has been stored for class @p cls. */
        inline size_t completed_runs(int cls) const { return s_runs.at(cls).size(); }
        /**
         * @brief Student-t confidence interval of the mean of the completed-run estimates.
         *
         * Requires at least two completed runs; returns [-DBL_MAX, DBL_MAX] otherwise.
         *
         * @param alpha  Significance level (e.g. 0.05 for a 95 % CI).
         * @param cls    Event-class index.
         * @return       Pair (lower bound, upper bound) of the CI.
         */
        inline pair<double, double> confidence_interval(double alpha, int cls) const
        {
            return conf_int(s_runs.at(cls), alpha);
        }
        /**
         * @brief Student-t confidence intervals across runs for all classes, in class order.
         */
        inline vector<pair<double, double>> confidence_interval(double alpha) const
        {
            vector<pair<double, double>> ci;
            for(size_t i = 0; i < s_runs.size(); i++)
            {
                ci.push_back(confidence_interval(alpha, static_cast<int>(i)));
            }
            return ci;
        }
        /**
         * @brief End the current run for a single class and optionally store its estimate.
         *
         * If @p newrun is @c true and the run has pairs with Σy ≠ 0, the run's estimate is
         * appended to the completed runs before the current run is cleared.
         */
        inline void reset(int cls, bool newrun) override
        {
            moments& m = cur.at(cls);
            if(newrun && m.k > 0 && m.my != 0.0)
            {
                s_runs.at(cls).push_back(m.mx / m.my);
            }
            m = moments();
        }
        /**
         * @brief End the current run for all classes and optionally store their estimates.
         */
        inline void reset(bool newrun) override
        {
            for(size_t i = 0; i < cur.size(); i++)
            {
                reset(static_cast<int>(i), newrun);
            }
        }
        /**
         * @brief Full reset: discard the current run and the completed-run estimates.
         */
        inline void clear() override
        {
            cur = vector<moments>(cur.size());
            s_runs = vector<vector<double>>(cur.size(), vector<double>());
        }
        /**
         * @brief Current-run estimates for all classes as a tab-separated string.
         */
        inline string to_string() const override
        {
            string str = "";
            for(size_t i = 0; i < cur.size(); i++)
            {
                str += std::to_string(get(static_cast<int>(i))) + "\t";
            }
            str += "\n";
            return str;
        }

    private:
        /**
         * @brief Current-run state of one class: pair count, means and co-moments
         *        Σ(x − x̄)², Σ(x − x̄)(y − ȳ), Σ(y − ȳ)².
         */
        struct moments
        {
            long   k   = 0;
            double mx  = 0.0;
            double my  = 0.0;
            double cxx = 0.0;
            double cxy = 0.0;
            double cyy = 0.0;
        };
        vector<moments>        cur;      ///< Current run, per class.
        vector<vector<double>> s_runs;   ///< Per-class store of completed-run estimates.
};

#endif
