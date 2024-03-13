#include "libdes_is.hpp"
namespace des{
	is::is() : policy("des::is")
	{}

	inline bool is::update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions)
	{
		if(l.size() < positions)
			return true;
		return false;
	}

	inline bool is::update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double rate)
	{
		return update(e,l, positions);
	}

	inline bool is::update(shared_ptr<event> e, list<shared_ptr<event>>& l, double rate)
	{
		throw runtime_error("des::is::update wrong function call: no rate parameter in is class");
		return false;
	}

	inline bool is::front()
	{
		return true;
	}
}