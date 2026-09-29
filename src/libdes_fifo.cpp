#include "libdes_fifo.hpp"
namespace des{
	fifo::fifo() : policy("des::fifo")
	{}

	unique_ptr<job_store> fifo::make_store() const
	{
		return make_unique<sequence_store>(true);
	}
}
