#include "libdes_node.hpp"

namespace des
{
	node::node() : object(id_gen++),
		q_map(),
		s_map(),
		q(),
		s(),
		gen(),
		in(),
		out(),
		usage(),
		last_event()
	{
		in.push_back(0);
		out.push_back(0);
		last_event.push_back(0);
		usage.push_back(0);
		q_map.push_back(vector<int>(1,0));
		s_map.push_back(vector<int>(1,0));
		observable_events.emplace(SIGNAL_NODE_ARRIVAL, list<shared_ptr<observer>>());
		observable_events.emplace(SIGNAL_NODE_DEPARTURE, list<shared_ptr<observer>>());
		observers = 0;
	}

	node::~node()
	{
		cout << "~node(): begin removing " << get_sid() << endl;
		if(observers > 0)
		{
			for(auto iter = observable_events.begin(); iter != observable_events.end(); iter++)
			{
				detach();
			}
			observable_events.clear();
			observers = 0;
		}
		cout << "~node(): done removing " << get_sid() << endl;
	}

	node::node(string desc) : node::node()
	{
		set_sid(desc);
	}

	node::node(string desc, shared_ptr<mt19937_64> g) : node::node(desc)
	{
		gen = g;
	}

	node::node(string desc, int cls) : node::node(desc)
	{
		for(unsigned int i = 0; i < cls - 1; i++)
		{
			in.push_back(0);
			out.push_back(0);
			last_event.push_back(0);
			usage.push_back(0);
		}
	}
	
	node::node(string desc, int cls, shared_ptr<mt19937_64> g) : node::node(desc, g)
	{
		for(unsigned int i = 0; i < cls - 1; i++)
		{
			in.push_back(0);
			out.push_back(0);
			last_event.push_back(0);
			usage.push_back(0);
		}
	}

	node::node(unsigned int cls, vector<shared_ptr<queue>> q, vector<shared_ptr<queue>> s, vector<vector<int>> qmap,
			vector<vector<int>> smap, string description, shared_ptr<mt19937_64> g): node::node(description, g)
	{
		for(unsigned int i = 1; i < cls - 1; i++)
		{
			in.push_back(0);
			out.push_back(0);
			last_event.push_back(0);
			usage.push_back(0);
		}
		q_map = qmap;
		s_map = smap;
	}

	node::node(unsigned int cls, string description, shared_ptr<mt19937_64> g) : node::node(description, g)
	{
		for(unsigned int i = 1; i < cls; i++)
		{
			in.push_back(0);
			out.push_back(0);
			last_event.push_back(0);
			usage.push_back(0);
		}
	}

	void node::reset(double time, vector<string> keys, bool newrun)
	{
		keys.push_back(NODE_ARRIVAL);
		for(unsigned int i =0; i < in.size(); i++)
		{
			in.at(i) = in.at(i) - out.at(i);
			out.at(i) = 0;
			if(last_event.at(i) - time > 0) 
			{
				last_event.at(i) = last_event.at(i) - time;
			}
			else
			{
				last_event.at(i) = 0;
			}
			usage.at(i) = 0;
		}
		for(shared_ptr<queue> qq: q)
		{
			qq -> reset(time, keys);
		}
		for(shared_ptr<queue> ss: s)
		{
			ss -> reset(time, keys);
		}
		for(pair<string, list<shared_ptr<observer>>> obs_l: observable_events)
		{
			for(shared_ptr<observer> obs: obs_l.second)
			{
				obs -> reset(newrun);
			}
		}
	}

	void node::clear()
	{
		for(unsigned int i =0; i < in.size(); i++)
		{
			in.at(i) = in.at(i) - out.at(i);
			out.at(i) = 0;
			last_event.at(i) = 0.0;
			usage.at(i) = 0;
		}
		for(shared_ptr<queue> qq: q)
		{
			qq -> clear();
		}
		for(shared_ptr<queue> ss: s)
		{
			ss -> clear();
		}
		for(pair<string, list<shared_ptr<observer>>> obs_l: observable_events)
		{
			for(shared_ptr<observer> obs: obs_l.second)
			{
				obs -> clear();
			}
		}
	}

	void node::update_in(unsigned int& cls, double& time)
	{
		if(in.at(cls) < out.at(cls))
		{
			throw runtime_error("node::arrival (node::id " + std::to_string(get_id()) +") there have been more departures than arrivals. Check des::event handling!");
		}
		// Compute the occupation of the node by job's class 
		usage.at(cls) += (in.at(cls) - out.at(cls)) * (time - last_event.at(cls));
		last_event.at(cls) = time;
		// Update the number of arrivals for the cls class
		++in.at(cls);
	}

	void node::update_out(unsigned int& cls, double& time)
	{
		if(in.at(cls) < out.at(cls))
		{
			throw runtime_error("node::arrival (node::id " + std::to_string(get_id()) +") there have been more departures than arrivals. Check des::event handling!");
		}
		// Compute the occupation of the node by job's class 
		usage.at(cls) += (in.at(cls) - out.at(cls)) * (time - last_event.at(cls));
		last_event.at(cls) = time;
		// Update the number of arrivals for the cls class
		++out.at(cls);
		// Compute the occupation of the node by job's class 
		usage.at(cls) += (in.at(cls) - out.at(cls)) * (time - last_event.at(cls));
		last_event.at(cls) = time;
	}

	int node::idx_next_departure() const
	{
		double min = __DBL_MAX__;
		int idx = 0, idx_min = 0;
		for(shared_ptr<queue> server: s)
		{
			if(server -> in_queue() != 0 && server -> min_time() < min)
			{
				min = server -> min_time();
				idx_min = idx;
			}
			++idx;
		}
		return idx_min;
	}

	double node::next_event_time() const
	{
		double min = __DBL_MAX__;
		for(shared_ptr<queue> server : s)
		{
			if(server -> in_queue() != 0 && server -> min_time() < min)
				min = server -> min_time();
		}
		return min;
	}

	bool node::arrival(shared_ptr<event>& e)
	{
		// object::TraceLoc(source_location::current(), "ARRIVAL ", std::to_string(e->get_id()), " time ",
		// 				 e->get_time(), " node ", std::to_string(get_id()));
		unsigned int cls = e -> get_cls();
		double time = e -> get_time();
		// Handle the new arrival 
		unsigned int sched = schedule(e, s_map);
		// 0) add the arrival time to the event's information
		e ->emplace_info(NODE_ARRIVAL, time);
		if(s.at(sched) -> is_full())
		{
			// object::TraceLoc(source_location::current(), "ENQUEUE ", std::to_string(e->get_id()), " time ",
			// 			 e->get_time(), " node ", std::to_string(get_id()));
			// There is no room in service, so wait for your turn in the queue
			// 1) choose which queue to enqueue the event
			int enq = enqueue(e, q_map);
			if(enq >= 0 && !q.at(enq)->is_full())
			{
				// 2) update counters 
				update_in(cls, time);
				// 3) enqueue event in the selected queue
				q.at(enq) -> enqueue(e, time);
				// 4) add the queue index to the event's information
				e ->emplace_info(EVENT_QUEUE, enq);
				// 5) add the arrival time to the event's information
				// e ->emplace_info(NODE_ARRIVAL, time);
				// 6) notify the arrival to observers
				if(observers != 0)
				{
					message msg(e -> get_map_info());
					notify(SIGNAL_NODE_ARRIVAL, msg);
				}
				return true;
			}
			else
			{
				// There is no room in the queue
				// TODO Count blocked events?
				return false;
			}
		}
		else
		{
			// object::TraceLoc(source_location::current(), "SERVICE ", std::to_string(e->get_id()), " time ",
			// 			 e->get_time(), " node ", std::to_string(get_id()));
			e -> emplace_info(EVENT_SERVER, sched);
			// 1) set the final time
			e -> set_time(time + get_service(cls, sched));
			// 2) add the arrival time to the event's information
			// e ->emplace_info(NODE_ARRIVAL, time);
			// 3) update counters
			update_in(cls, time); 
			// 4) enqueue event in the selected queue
			if(!s.at(sched) -> enqueue(e, time))  // sched was enq
			{
				throw runtime_error("node::arrival (node::id " + std::to_string(get_id()) +") error scheduling of the current event in queue.\n" + e ->to_string());
			}
			// 5) notify the arrival to observers
			if(observers != 0)
			{
				message msg(e -> get_map_info());
				notify(SIGNAL_NODE_ARRIVAL, msg);
			}
			return true;
		}	
	}

	shared_ptr<event> node::departure()
	{
		// Find the queue where holding the event to handle
		int deq = idx_next_departure();
		if(s.at(deq) -> in_queue() != 0)
		{
			// Get the event
			shared_ptr<event> e = s.at(deq) -> dequeue();
			// object::TraceLoc(source_location::current(), "DEPARTURE ", std::to_string(e->get_id()), " time ",
			// 				 e -> get_time(), " node ", std::to_string(get_id()));
			// ..., its class id
			unsigned int cls = e -> get_cls();
			// ... and the current time
			double time = e -> get_time();
			// Update counters
			e -> emplace_info(NODE_SOJOURN, time - e -> get_info(NODE_ARRIVAL).second);
			// notify the departure to observers
			if(observers != 0)
			{
				message msg(e -> get_map_info());
				notify(SIGNAL_NODE_DEPARTURE, msg);
			}
			// Try to schedule another event in service
			unsigned int sched = schedule(e, s_map);
			if(!s.at(sched) -> is_full())
			{
				// 1) Find the index of the queue to pick the next event to serve
				deq = dequeue(e, q_map);
				if(q.size() > 0 && q.at(deq) ->in_queue() != 0)
				{
					// 2) get the event
					shared_ptr<event> ev = q.at(deq) -> dequeue();
					// 3) Update counters
					unsigned int mvcls = ev -> get_cls();
					// 4) Set the final time
					ev -> set_time(time + get_service(mvcls, sched));	
					// 5) Find the index of the serverSet the server id
					ev -> emplace_info(EVENT_SERVER, sched);
					s.at(sched) -> enqueue(ev, time);
					//.at(sched).at(mvcls);
				}
				update_out(cls, time);
				// remove the queue id and server id from the event information
				e -> remove_info(EVENT_QUEUE);
				e -> remove_info(EVENT_SERVER);
				e -> remove_info(NODE_ARRIVAL);
				return e;
			}
			else
			{
				throw runtime_error("node::departure (node::id " + std::to_string(get_id()) +") error in handling the scheduling of the event.\n");
			}
		}
		else
		{
			throw runtime_error("node::departure (node::id " + std::to_string(get_id()) +") error dequeuing current event.\n");
		}
	}

	string node::to_string() const
	{
		string str = get_sid();
		if(q_map.size() != 0)
		{
			str += "\n\tQueue:";
			for(unsigned int i = 0; i < q_map.size(); i++)
			{
				// Write the header of the table
				if(i == 0)
				{
					str+= "\n\t#\t";
					for(unsigned int j = 0; j < q_map.at(0).size(); j++)
					{
						str += "cls " + std::to_string(j) + "\t";
					}
					str += "policy\n";
				}
				str += "\n\t" + std::to_string(i) + "\t";
				for(unsigned int j = 0; j < q_map.at(i).size(); j++)
				{
					str+= std::to_string(q_map.at(i).at(j)) + "\t";
				}
				str += q.at(i) -> get_policy() + "\n";
			}
		}
		if(s_map.size() != 0)
		{
			str += "\n\tService:";
			for(unsigned int i = 0; i < s_map.size(); i++)
			{
				// Write the header of the table
				if(i == 0)
				{
					str+= "\n\t#\t";
					for(unsigned int j = 0; j < s_map.at(0).size(); j++)
					{
						str += "cls " + std::to_string(j) + "\t";
					}
					str += "policy\n";
				}
				str += "\n\t" + std::to_string(i) + "\t";
				for(unsigned int j = 0; j < s_map.at(i).size(); j++)
				{
					str+= std::to_string(s_map.at(i).at(j)) + "\t";
				}
				str += s.at(i) -> get_policy() + "\n";
			}
		}
		str += "\tThroughput\n";
		for(unsigned int i = 0; i < last_event.size(); i++)
		{
			string nm = "dep_" + std::to_string(i);
		}
		if(last_event.size() >= 1)
		{
			double thr = 0.0;
			for(unsigned int i = 0; i < last_event.size(); i++)
			{
				thr += out.at(i); 
			}
			thr = thr/last_event.at(last_event.size()-1);
			str += "\t\t" + std::to_string(thr) + "\n";
		}
		return str;
	}

}