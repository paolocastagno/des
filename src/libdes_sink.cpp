#include "libdes_sink.hpp" 

namespace des
{
	sink::sink(string description, int cls) : sourcesink(description, cls)
	{
		set_sid(description);
		s.push_back(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo()))));
	}

	sink::sink(string description) : sink(description, 1)
	{}

	sink::sink(vector<double> r, string description, mt19937_64&) : sink(description, static_cast<int>(r.size()))
	{}

	sink::~sink()
	{}

	int sink::schedule(const shared_ptr<event>&, const vector<vector<int>>&)
	{
		// It does not matter the class of the incoming job, all jobs get cleared and
		// disposed in the events list for future use.
		// Since the queue for services has no room, all incoming jobs are handled as
		// losses.
		return 0;
	}

	int sink::enqueue(const shared_ptr<event>&, const vector<vector<int>>&)
	{
		// It does not matter the class of the incoming job, all jobs get cleared and
		// disposed in the events list for future use
		return 0;
	}

	int sink::dequeue(const shared_ptr<event>& e, const vector<vector<int>>&)
	{
		dispose_event(e);
		return 0;
	}

	string sink::to_string() const
	{
		return "node::sink::" + node::to_string();
	}
}
