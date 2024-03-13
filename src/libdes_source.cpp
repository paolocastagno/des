#include "libdes_source.hpp" 

namespace des
{
	source::source(vector<double> r, string description, shared_ptr<mt19937_64> gen) : sourcesink(description, r.size(), gen),
		rate(r),
		exp()
	{
		vector<pair<int, vector<int>>> s_map;
		// for(vector<double>::iterator it = r.begin(); it != r.end(); it++)
		q.clear();
		q_map.clear();
		for(unsigned int i = 0; i < r.size(); i++)
		{
			// Set the average service rate for each event class
			exp.push_back(exponential_distribution<double>(r.at(i)));
			// Setup the mapping between event class and queue where jobs are enqueued
			// q_map.push_back(make_pair<int, vector<int>>(0, vector<int>(r.size(), 1)));
			// Setup the mapping between event class and server where jobs are served
			s_map.push_back(make_pair<int, vector<int>>(0, vector<int>(r.size(), 1)));
			// Setup a queue for serve jobs of a given class
			s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
		}
		// q.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	source::source(string description) : sourcesink::sourcesink(description),
		exp()
	{
		set_sid(description);
		vector<pair<int, vector<int>>> s_map;
		// Setup the mapping between event class and queue where jobs are enqueued
		// q_map.push_back(make_pair<int, vector<int>>(0, vector<int>(1, 1)));
		// Setup the mapping between event class and server where jobs are served
		s_map.push_back(make_pair<int, vector<int>>(0, vector<int>(1, 1)));
		s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
		q.clear();
		q_map.clear();
		// q.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	double source::get_service(unsigned int& cls, unsigned int& idx)
	{
		return exp.at(cls)(*gen.get());
	}

	int source::enqueue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		// object::TraceLoc(source_location::current(), std::to_string(e->get_id()), " time ",
		// 				 e->get_time(), " node ", std::to_string(get_id()));
		return 0;
	}

	int source::dequeue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		// object::TraceLoc(source_location::current(), std::to_string(e->get_id()), " time ",
		// 				 e->get_time(), " node ", std::to_string(get_id()));
		shared_ptr<event> ev = sourcesink::get_event();
		ev -> clone(*e);
		ev -> set_time(ev -> get_time());// + get_service(cls));
		arrival(ev);
		return 0;
	}

	int source::schedule(shared_ptr<event>& e, vector<vector<int>> s_map)
	{
		return 0;
	}

	string source::to_string() const
	{
		return "node::source::" + node::to_string();
	}
}