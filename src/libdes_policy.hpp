#ifndef POLICY_H
#define POLICY_H

#include <string>
#include <memory>

#include "libdes_object.hpp"
#include "libdes_event.hpp"
#include "libdes_store.hpp"


using namespace std;

namespace des
{
	class policy;
}

/**
 * @brief Queueing discipline of a des::queue.
 *
 * A policy decides which jobs a queue admits (admit()) and how the admitted jobs are
 * stored and released (make_store()). A policy may be shared by several queues: each
 * queue asks it for its own store when it is built, so the policy itself holds no jobs.
 */
class des::policy : public des::object
{
	public:
		policy(string d) : object(id_gen++)
		{
			description = d;
		}
		/**
		 * @brief Creates the store holding the jobs of one queue that uses this policy
		 */
		virtual unique_ptr<job_store> make_store() const = 0;
		/**
		 * @brief Tells whether a queue with @p positions places, currently holding @p jobs,
		 * admits job @p e. By default a job is admitted while there is a free place.
		 */
		virtual bool admit(const event&, const job_store& jobs, unsigned int positions) const
		{
			return jobs.size() < positions;
		}
		inline string get_description() const
		{
			return description;
		}

		virtual ~policy(){};

		inline void set_description(string str)
		{
			description = str;
		}

		virtual inline string to_string() const override {
			string s = "\tdes::policy (" + std::to_string(get_id()) + ")\t" + description + "\n";
			return s;
		}

		virtual inline void clear() override
		{}

	private:
		string description;
		inline static atomic<unsigned int> id_gen{0};
};


#endif
