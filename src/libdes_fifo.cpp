#include "libdes_fifo.hpp"
namespace des{
	fifo::fifo() : policy("des::fifo")
	{}

	inline bool fifo::update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions)
	{
		if(l.size() < positions)
			return true;
		return false;
	}

	inline bool fifo::update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double rate)
	{
		return update(e,l, positions);
	}

	inline bool fifo::update(shared_ptr<event> e, list<shared_ptr<event>>& l, double rate)
	{
		throw runtime_error("des::fifo::update wrong function call: no rate parameter in fifo class");
		return false;
	}

	inline bool fifo::front()
	{
		return true;
	}
}