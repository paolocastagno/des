#include "libdes_sink.hpp" 

namespace des
{
	sink::sink(string description, int cls) : sourcesink(description, cls)
	{
		set_sid(description);
		vector<pair<int, vector<int>>> s_map;
		// Setup the mapping between event class and server where jobs are served
		q.clear();
		q_map.clear();
		s_map.push_back(make_pair<int, vector<int>>(1, vector<int>(1, 1)));
		s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	sink::sink(string description) : sink(description, 1)
	{
		set_sid(description);
		vector<pair<int, vector<int>>> s_map;
		// Setup the mapping between event class and server where jobs are served
		q.clear();
		q_map.clear();
		s_map.push_back(make_pair<int, vector<int>>(1, vector<int>(1, 1)));
		s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	sink::~sink()
	{}

	int sink::schedule(shared_ptr<event>& e, vector<vector<int>> s_map)
	{
		// It does not matter the class of the incoming job, all jobs get cleared and
		// disposed in the events list for future use.
		// Since the queue for services has no room, all incoming jobs are handled as
		// losses.
		return 0;
	}

	int sink::enqueue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		// It does not matter the class of the incoming job, all jobs get cleared and
		// disposed in the events list for future use
		return 0;
	}

	int sink::dequeue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		dispose_event(e);
		return 0;
	}

	string sink::to_string() const
	{
		return "node::sink::" + node::to_string();
	}
}