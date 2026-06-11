#include "libdes_network.hpp"

namespace des
{
	network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
					shared_ptr<mt19937_64>& g) :
		nodes(nds),
		routing(rtg.begin(), rtg.end()),
		// handler(), 
		gen(g)
		{
			set_sid("Network");
			// Initialaize function pointers to nullptr and the default functions will be used;
			handle_block = nullptr;
			handle_forks = nullptr;
			for(unsigned int i = 0; i < routing.size(); i++)
			{
				vector<shared_ptr<counter>> cnt;
				vector<shared_ptr<scalar>> flw;
				for(unsigned int j = 0; j < routing.at(i).size(); j++)
				{
					int cls = routing.at(i).at(0).size();
					string nm = "_" + std::to_string(i) + "_" + std::to_string(j);
					observable_events.emplace(SIGNAL_NET_ROUTING+nm, list<shared_ptr<observer>>());
					attach(SIGNAL_NET_ROUTING+nm, shared_ptr<counter>(new counter("count"+nm,cls)));
					attach(SIGNAL_NET_ROUTING+nm, shared_ptr<scalar>(new scalar("flow"+nm,cls)));
				}
			}
			init_heap();
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
						int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
						shared_ptr<mt19937_64>& g) : network::network(nds, rtg, g)
		{
			handle_forks = hffunc;
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
						pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
												const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
						shared_ptr<mt19937_64>& g) : network::network(nds, rtg, g)
		{
			handle_block = hbfunc;
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
							int (*hffunc)(shared_ptr<event>, const vector<vector<vector<double>>>&,
										shared_ptr<mt19937_64>&),
							pair<bool, int> (*hbfunc)(shared_ptr<event>, int,
							const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&),
						shared_ptr<mt19937_64>& g) : network::network(nds, rtg, g)
		{
			handle_forks = hffunc;
			handle_block = hbfunc;
		}

		bool network::insert(shared_ptr<event> e, string ndesc)
		{
			unsigned int i =0;
			while(i < nodes.size() && nodes.at(i) -> get_sid() != ndesc)
			{
				++i;
			}
			if(nodes.at(i) -> get_sid() == ndesc)
			{
				return nodes.at(i) -> arrival(e);
			}
			else
			{
				return false;
			}
		}

	// void network::handle_constraints(shared_ptr<event> e, const double& time)
	// {
	// 	bool keep_going = true;
	// 	int node = static_cast<int>(e -> get_info(EVENT_CURRENT_NODE).second), cls = e -> get_cls();
	// 	vector<pair<shared_ptr<Constraint>, Handler>> node_cons = handler.at(node).at(cls);
	// 	size_t i = 0;
	// 	while(keep_going && i < node_cons.size())
	// 	{
	// 		Constraint c = *(node_cons.at(i).first.get());
	// 		Handler h = node_cons.at(i).second;
	// 		event *ev = e.get();
	// 		if(c(ev))
	// 		{
	// 			if(!h(ev, nodes))
	// 			{
	// 				count_constrained_events.at(node).at(cls).at(i) += 1;
	// 				keep_going = false;
	// 			}
	// 		}
	// 		++i;
	// 	}
	// }

	shared_ptr<event> network::next_event()
	{
		if(event_heap.empty())
		{
			init_heap();
			if(event_heap.empty()) return nullptr;
		}
		auto it = event_heap.begin();
		int idx = it->second;
		event_heap.erase(it);
		node_heap_time[idx] = __DBL_MAX__;
		shared_ptr<event> e = nodes.at(idx)->departure();
		update_heap(idx);
		return e;
	}

	void network::init_heap()
	{
		event_heap.clear();
		node_heap_time.assign(nodes.size(), __DBL_MAX__);
		for(unsigned int i = 0; i < nodes.size(); i++)
		{
			node_heap_time[i] = nodes.at(i)->next_event_time();
			if(has_pending_event(node_heap_time[i]))
			{
				event_heap.emplace(node_heap_time[i], i);
			}
		}
	}

	void network::update_heap(int idx)
	{
		event_heap.erase({node_heap_time[idx], idx});
		node_heap_time[idx] = nodes.at(idx)->next_event_time();
		if(has_pending_event(node_heap_time[idx]))
		{
			event_heap.emplace(node_heap_time[idx], idx);
		}
	}

	pair<bool, int> network::hbfunc(shared_ptr<event>, int, const vector<vector<vector<double>>>&, shared_ptr<mt19937_64>&)
	{
		return make_pair<bool, int>(false, 0);
	}
	
	int network::hffunc(shared_ptr<event> e, const vector<vector<vector<double>>>& route, shared_ptr<mt19937_64>& g)
	{
		uniform_real_distribution<double> dist;
		int i = 0;
		double rnd = dist(*g.get()), cum = 0.0;
		vector<vector<double>> rtg_vec = route.at(e -> get_info(EVENT_NODE).second);
		do
		{
			cum += rtg_vec.at(i).at(e -> get_cls());
		}
		while(cum < rnd && ++i < static_cast<int>(route.size()));
		if(cum > rnd)
		{
			return i;
		}
		else
		{
			return -1;
		}
	}

	void network::route(shared_ptr<event> e)
	{
		pair<bool, double> node = e -> get_info(EVENT_NODE);
		if(!node.first)
		{
			throw runtime_error("network::route failed to recover the current node id\n");
		}
		else
		{
			int source = static_cast<int>(node.second);
			int dest;
			if(handle_forks != nullptr)
			{
				dest = handle_forks(e, routing, gen);
			}
			else
			{
				dest = hffunc(e, routing, gen);
			}

			if(dest > 0  && static_cast<unsigned int>(dest) < routing.at(source).size())
			{
				e -> emplace_info(EVENT_NODE, dest);
				if(nodes.at(dest) -> arrival(e))
				{
					// Count the routing choice
					string nm = "_" + std::to_string(source) + "_" + std::to_string(dest);
					if(observers != 0)
					{
						std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(SIGNAL_NET_ROUTING+nm);
						if(fnd != observable_events.end())
						{
							std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
							double flw = static_cast<double>(dynamic_cast<counter*>((*lst).get()) -> get(e->get_cls()) + 1) / e->get_time();
							unordered_map<string,double> info = e->get_map_info();
							info.insert({"flow"+nm, flw});
							message m(info);
							notify(SIGNAL_NET_ROUTING+nm,m);
						}
					}
					else
					{
						throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
					}
					// Event successfully enqueued; update the heap for the destination node
					update_heap(dest);
				}
				else
				{
					e -> emplace_info(EVENT_NODE, source);
					// handle block
					if(handle_block != nullptr)
					{
						e -> set_info(EVENT_REJECT, 1.0);
						pair<bool, int> reroute;
						bool cond = false;
						do
						{
							reroute = handle_block(e, dest, routing, gen);
							if(reroute.first)
							{
								// Count the routing choice
								string nm = "_" + std::to_string(source) + "_" + std::to_string(dest);
								if(observers != 0)
								{
									std::unordered_map<string,std::list<std::shared_ptr<observer>>>::iterator fnd = observable_events.find(SIGNAL_NET_ROUTING+nm);
									if(fnd != observable_events.end())
									{
										std::list<shared_ptr<observer>>::iterator lst = fnd->second.begin();
										double flw = static_cast<double>(dynamic_cast<counter*>((*lst).get()) -> get(e->get_cls()) + 1) / e->get_time();
										unordered_map<string,double> info = e->get_map_info();
										info.insert({"flow"+nm, flw});
										message m(info);
										notify(SIGNAL_NET_ROUTING+nm,m);
									}
								}
								else
								{
									throw invalid_argument("Measurable event " + nm + " is not defined in network " + get_sid());
								}
								e -> emplace_info(EVENT_NODE, reroute.second);
								cond = nodes.at(reroute.second) -> arrival(e);
								if(cond)
								{
									update_heap(reroute.second);
								}
								else
								{
									e -> emplace_info(EVENT_NODE, source);
									reroute.first = false;
								}
							}
						}
						while (!cond && reroute.first);
					}
				}
			}
		}
	}

	void network::reset(double time, vector<string> keys, bool newrun)
	{
		for(std::unordered_map<string,list<shared_ptr<observer>>>::iterator it = observable_events.begin(); it != observable_events.end(); it++)
		{
			for(shared_ptr<observer> obs: it -> second)
			{
				obs -> reset(newrun);
			}
		}
		for(shared_ptr<node> n: nodes)
		{
			n -> reset(time, keys, newrun);
		}
		init_heap();
	}

	string network::to_string() const
	{
		string s = "des::network \n\tRouting table";
		for(unsigned int i = 0; i < routing.at(0).size(); i++)
		{
			s += "\t" + std::to_string(i) + "\t";
		}
		for(unsigned int i = 0; i < routing.size(); i++)
		{
			s += "\t" + std::to_string(i) + "\t";
			for(unsigned int j = 0; j < routing.at(i).size(); j++)
			{
				s += "(";
				for(unsigned int k = 0; k < routing.at(i).at(j).size(); k++)
				{
					s += std::to_string(routing.at(i).at(j).at(k));
					if(k < routing.at(i).at(j).size() -1)
					{
						s += " ";
					}
				}
				s += ")\t";
			}
			s+="\n";
		}
		return s;
	}
}
