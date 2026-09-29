#include "libdes_is.hpp"
namespace des{
	is::is() : policy("des::is")
	{}

	unique_ptr<job_store> is::make_store() const
	{
		return make_unique<time_store>();
	}

	bool is::admit(const event&, const job_store&, unsigned int) const
	{
		// IS: infinite server — always admit, regardless of configured capacity.
		return true;
	}
}
