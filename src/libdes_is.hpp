#ifndef IS_H
#define IS_H

#include <memory>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class is;
}

/**
 * @brief Infinite server discipline: every job is admitted, regardless of the configured
 * capacity, and jobs are released in order of completion time.
 */
class des::is : public des::policy
{
	public:
		/**
		 * @brief Construct a new Infinite Servre (is) object
		 *
		 */
		is();
		/**
		 * @brief Stores the jobs in a heap ordered by completion time
		 */
		unique_ptr<job_store> make_store() const override;
		/**
		 * @brief Always admits
		 */
		bool admit(const event& e, const job_store& jobs, unsigned int positions) const override;
};
#endif
