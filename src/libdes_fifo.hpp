#ifndef FIFO_H
#define FIFO_H

#include <memory>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class fifo;
}

/**
 * @brief First-in first-out discipline: jobs are released in time order, ties in arrival order.
 *
 * In a waiting queue a job's time is its arrival time; in a server it is its completion
 * time, so a multi-place server releases jobs as they complete.
 */
class des::fifo : public des::policy
{
	public:
		fifo();
		unique_ptr<job_store> make_store() const override;
};
#endif
