#include "libdes_is.hpp"
namespace des{
	is::is() : policy("des::is")
	{}

	inline bool is::update(shared_ptr<event>, list<shared_ptr<event>>&, unsigned int)
	{
		// IS: infinite server — always admit, regardless of configured capacity.
		return true;
	}

	inline bool is::update(shared_ptr<event>, list<shared_ptr<event>>&, unsigned int, double)
	{
		// IS: always admit.
		return true;
	}

	inline bool is::update(shared_ptr<event>, list<shared_ptr<event>>&, double)
	{
		throw runtime_error("des::is::update wrong function call: no rate parameter in is class");
		return false;
	}

	inline bool is::front()
	{
		return true;
	}
}
