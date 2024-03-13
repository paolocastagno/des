#ifndef FIFO_H
#define FIFO_H

#include <list>
#include <memory>

#include "libdes_event.hpp"
#include "libdes_policy.hpp"

using namespace std;

namespace des
{
	class fifo;
}

class des::fifo : public des::policy
{
	public:
		fifo();
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double rate);
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions);
		bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, double rate);
		bool front();
};
#endif