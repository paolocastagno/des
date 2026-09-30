#include "libdes_network.hpp"

namespace des
{
	network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
					uint64_t seed) :
		nodes(nds),
		routing(rtg.begin(), rtg.end()),
		// handler(), 
		route_streams()
		{
			set_sid("Network");
			// Block 2i feeds node i, block 2i+1 routes the events leaving it
			random_engine block(seed);
			for(const shared_ptr<node>& n: nodes)
			{
				n -> set_streams(block);
				block.long_jump();
				route_streams.emplace_back(block);
				block.long_jump();
			}
			// Function pointers default to nullptr and the default functions will be used;
			max_reroute_attempts = max<unsigned int>(1, nodes.size());
			for(unsigned int i = 0; i < routing.size(); i++)
			{
				// Events leaving node i may belong to any class routed on one of its edges
				int node_cls = 1;
				for(unsigned int j = 0; j < routing.at(i).size(); j++)
				{
					// Each edge tracks the classes listed in its own routing entry
					int cls = routing.at(i).at(j).size();
					node_cls = max(node_cls, cls);
					string nm = edge(i, j);
					observable_events.emplace(SIGNAL_NET_ROUTING+nm, list<shared_ptr<observer>>());
					attach(SIGNAL_NET_ROUTING+nm, shared_ptr<counter>(new counter("count"+nm,cls)));
					observable_events.emplace(SIGNAL_NET_FLOW+nm, list<shared_ptr<observer>>());
					attach(SIGNAL_NET_FLOW+nm, shared_ptr<scalar>(new scalar("flow"+nm,cls)));
					observable_events.emplace(SIGNAL_NET_BLOCK+nm, list<shared_ptr<observer>>());
					attach(SIGNAL_NET_BLOCK+nm, shared_ptr<counter>(new counter("blocked"+nm,cls)));
				}
				string nm = "_" + std::to_string(i);
				observable_events.emplace(SIGNAL_NET_LOSS+nm, list<shared_ptr<observer>>());
				attach(SIGNAL_NET_LOSS+nm, shared_ptr<counter>(new counter("lost"+nm,node_cls)));
			}
			index_signals();
			init_heap();
			build_route_table();
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
						int (*hffunc)(const shared_ptr<event>&, const vector<vector<vector<double>>>&, random_engine&),
						uint64_t seed) : network::network(nds, rtg, seed)
		{
			handle_forks = hffunc;
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
						pair<bool, int> (*hbfunc)(const shared_ptr<event>&, int,
												const vector<vector<vector<double>>>&, random_engine&),
						uint64_t seed) : network::network(nds, rtg, seed)
		{
			handle_block = hbfunc;
		}

		network::network(vector<shared_ptr<node>> nds, vector<vector<vector<double>>> rtg,
							int (*hffunc)(const shared_ptr<event>&, const vector<vector<vector<double>>>&,
										random_engine&),
							pair<bool, int> (*hbfunc)(const shared_ptr<event>&, int,
							const vector<vector<vector<double>>>&, random_engine&),
						uint64_t seed) : network::network(nds, rtg, seed)
		{
			handle_forks = hffunc;
			handle_block = hbfunc;
		}

		bool network::insert(const shared_ptr<event>& e, string ndesc)
		{
			unsigned int i =0;
			while(i < nodes.size() && nodes.at(i) -> get_sid() != ndesc)
			{
				++i;
			}
			if(i < nodes.size() && nodes.at(i) -> arrival(e))
			{
				update_heap(i);
				return true;
			}
			return false;
		}

	// void network::handle_constraints(const shared_ptr<event>& e, const double& time)
	// {
	// 	bool keep_going = true;
	// 	int node = static_cast<int>(e -> get_info(tags::EVENT_CURRENT_NODE).second), cls = e -> get_cls();
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
		int idx = event_heap.front();
		shared_ptr<event> e = nodes.at(idx)->departure();
		update_heap(idx);
		now = e->get_time();
		return e;
	}

	void network::init_heap()
	{
		event_heap.clear();
		heap_pos.assign(nodes.size(), -1);
		node_heap_time.assign(nodes.size(), __DBL_MAX__);
		for(unsigned int i = 0; i < nodes.size(); i++)
		{
			double t = nodes.at(i)->next_event_time();
			if(has_pending_event(t))
			{
				node_heap_time[i] = t;
				heap_pos[i] = static_cast<int>(event_heap.size());
				event_heap.push_back(i);
			}
		}
		for(size_t i = event_heap.size() / 2; i-- > 0;)
		{
			heap_sift_down(i);
		}
	}

	void network::update_heap(int idx)
	{
		double t = nodes.at(idx)->next_event_time();
		int p = heap_pos[idx];
		if(!has_pending_event(t))
		{
			node_heap_time[idx] = __DBL_MAX__;
			if(p >= 0)
			{
				// Replace the entry with the last one and restore the order around it
				size_t last = event_heap.size() - 1;
				if(static_cast<size_t>(p) != last)
				{
					heap_swap(p, last);
				}
				event_heap.pop_back();
				heap_pos[idx] = -1;
				if(static_cast<size_t>(p) < event_heap.size())
				{
					int moved = event_heap[p];
					heap_sift_up(p);
					heap_sift_down(heap_pos[moved]);
				}
			}
			return;
		}
		if(p >= 0 && node_heap_time[idx] == t)
		{
			// Unchanged: the heap is already in order
			return;
		}
		node_heap_time[idx] = t;
		if(p < 0)
		{
			heap_pos[idx] = static_cast<int>(event_heap.size());
			event_heap.push_back(idx);
			heap_sift_up(event_heap.size() - 1);
		}
		else
		{
			heap_sift_up(p);
			heap_sift_down(heap_pos[idx]);
		}
	}

	void network::heap_sift_up(size_t i)
	{
		while(i > 0)
		{
			size_t parent = (i - 1) / 2;
			if(!heap_less(event_heap[i], event_heap[parent]))
			{
				break;
			}
			heap_swap(i, parent);
			i = parent;
		}
	}

	void network::heap_sift_down(size_t i)
	{
		size_t n = event_heap.size();
		while(true)
		{
			size_t smallest = i, l = 2 * i + 1, r = l + 1;
			if(l < n && heap_less(event_heap[l], event_heap[smallest]))
			{
				smallest = l;
			}
			if(r < n && heap_less(event_heap[r], event_heap[smallest]))
			{
				smallest = r;
			}
			if(smallest == i)
			{
				break;
			}
			heap_swap(i, smallest);
			i = smallest;
		}
	}

	void network::index_signals()
	{
		edge_lists.assign(routing.size(), vector<edge_signals>());
		loss_lists.assign(routing.size(), nullptr);
		auto lookup = [this](const string& signal) -> list<shared_ptr<observer>>*
		{
			auto it = observable_events.find(signal);
			return it == observable_events.end() ? nullptr : &(it -> second);
		};
		for(unsigned int i = 0; i < routing.size(); i++)
		{
			edge_lists[i].resize(routing.at(i).size());
			for(unsigned int j = 0; j < routing.at(i).size(); j++)
			{
				edge_lists[i][j].route = lookup(SIGNAL_NET_ROUTING + edge(i, j));
				edge_lists[i][j].block = lookup(SIGNAL_NET_BLOCK + edge(i, j));
			}
			loss_lists[i] = lookup(SIGNAL_NET_LOSS + "_" + std::to_string(i));
		}
		lists_owner = this;
	}

	void network::build_route_table()
	{
		route_table.assign(routing.size(), vector<vector<pair<int, double>>>());
		for(size_t i = 0; i < routing.size(); i++)
		{
			// Events leaving node i may belong to any class routed on one of its edges
			size_t classes = 0;
			for(const vector<double>& edge_cls: routing[i])
			{
				classes = max(classes, edge_cls.size());
			}
			route_table[i].resize(classes);
			for(size_t c = 0; c < classes; c++)
			{
				vector<pair<int, double>> row;
				double cum = 0.0;
				bool complete = true;
				for(size_t j = 0; j < routing[i].size() && complete; j++)
				{
					// Each edge lists its own classes: leave the row to the scan if one misses c
					complete = c < routing[i][j].size();
					if(complete && routing[i][j][c] != 0.0)
					{
						cum += routing[i][j][c];
						row.emplace_back(static_cast<int>(j), cum);
					}
				}
				if(complete)
				{
					route_table[i][c] = std::move(row);
				}
			}
		}
	}

	pair<bool, int> network::hbfunc(const shared_ptr<event>&, int, const vector<vector<vector<double>>>&, random_engine&)
	{
		return make_pair<bool, int>(false, 0);
	}
	
	int network::hffunc(const shared_ptr<event>& e, const vector<vector<vector<double>>>& route, random_engine& g)
	{
		uniform_real_distribution<double> dist;
		int i = 0;
		double rnd = dist(g), cum = 0.0;
		const size_t source = static_cast<size_t>(e -> get_info(tags::EVENT_NODE).second);
		const int cls = e -> get_cls();
		// The table holds the partial sums of the scan below at its nonzero entries, so it
		// yields the same destination; with rnd = 0 the scan may stop on a zero entry instead
		if(&route == &routing && rnd > 0.0 && source < route_table.size()
			&& static_cast<size_t>(cls) < route_table[source].size() && !route_table[source][cls].empty())
		{
			for(const pair<int, double>& dest: route_table[source][cls])
			{
				if(dest.second >= rnd)
				{
					return dest.second > rnd ? dest.first : -1;
				}
			}
			return -1;
		}
		const vector<vector<double>>& rtg_vec = route.at(source);
		do
		{
			cum += rtg_vec.at(i).at(cls);
		}
		while(cum < rnd && ++i < static_cast<int>(rtg_vec.size()));
		if(cum > rnd)
		{
			return i;
		}
		else
		{
			return -1;
		}
	}

	void network::notify_routing(const shared_ptr<event>& e, int source, int dest)
	{
		if(lists_owner != this)
		{
			index_signals();
		}
		if(static_cast<unsigned int>(dest) >= edge_lists.at(source).size())
		{
			return;
		}
		list<shared_ptr<observer>>* obs = edge_lists[source][dest].route;
		if(obs == nullptr || obs -> empty())
		{
			return;
		}
		message m = message::view_of(e -> get_store());
		deliver(obs, m);
	}

	void network::notify_class(list<shared_ptr<observer>>* obs, int cls)
	{
		if(obs == nullptr || obs -> empty())
		{
			return;
		}
		message m;
		m.add(tags::EVENT_CLS, cls);
		deliver(obs, m);
	}

	bool network::try_arrival(const shared_ptr<event>& e, int source, int dest)
	{
		e -> emplace_info(tags::EVENT_NODE, dest);
		if(nodes.at(dest) -> arrival(e))
		{
			notify_routing(e, source, dest);
			// Event successfully enqueued; update the heap for the destination node
			update_heap(dest);
			return true;
		}
		e -> emplace_info(tags::EVENT_NODE, source);
		if(lists_owner != this)
		{
			index_signals();
		}
		if(static_cast<unsigned int>(dest) < edge_lists.at(source).size())
		{
			notify_class(edge_lists[source][dest].block, e -> get_cls());
		}
		return false;
	}

	void network::route(const shared_ptr<event>& e)
	{
		pair<bool, double> node = e -> get_info(tags::EVENT_NODE);
		if(!node.first)
		{
			throw runtime_error("network::route failed to recover the current node id\n");
		}
		int source = static_cast<int>(node.second);
		random_engine& g = route_streams.at(source).at(static_cast<size_t>(e -> get_cls()));
		int dest;
		if(handle_forks != nullptr)
		{
			dest = handle_forks(e, routing, g);
		}
		else
		{
			dest = hffunc(e, routing, g);
		}
		// A destination outside the routing row means the event leaves the network
		if(dest < 0 || static_cast<unsigned int>(dest) >= routing.at(source).size())
		{
			return;
		}
		if(try_arrival(e, source, dest))
		{
			return;
		}
		// The destination is full: the block handler may reroute the event, a bounded number of times
		e -> emplace_info(tags::EVENT_REJECT, 1.0);
		int blocked = dest;
		for(unsigned int attempt = 0; attempt < max_reroute_attempts; ++attempt)
		{
			pair<bool, int> reroute;
			if(handle_block != nullptr)
			{
				reroute = handle_block(e, blocked, routing, g);
			}
			else
			{
				reroute = hbfunc(e, blocked, routing, g);
			}
			if(!reroute.first || reroute.second < 0 || static_cast<unsigned int>(reroute.second) >= nodes.size())
			{
				break;
			}
			if(try_arrival(e, source, reroute.second))
			{
				return;
			}
			blocked = reroute.second;
		}
		// No destination accepted the event: it is lost
		if(lists_owner != this)
		{
			index_signals();
		}
		notify_class(loss_lists.at(source), e -> get_cls());
	}

	void network::reset(double time, vector<tag> keys, bool newrun)
	{
		// Close the run: each edge contributes its throughput N/T to the flow estimate
		if(time > 0)
		{
			for(unsigned int i = 0; i < routing.size(); i++)
			{
				for(unsigned int j = 0; j < routing.at(i).size(); j++)
				{
					counter* cnt = find_front<counter>(SIGNAL_NET_ROUTING + edge(i, j));
					scalar* flw = find_front<scalar>(SIGNAL_NET_FLOW + edge(i, j));
					if(cnt != nullptr && flw != nullptr)
					{
						for(unsigned int k = 0; k < routing.at(i).at(j).size(); k++)
						{
							flw -> update(cnt -> get(k) / time, k);
						}
					}
				}
			}
		}
		now = now > time ? now - time : 0.0;
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
