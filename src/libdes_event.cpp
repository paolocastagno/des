#include "libdes_event.hpp"

namespace des
{
	event::event() : object(id_gen++),
			store()
	{
		store.reserve(tags::BUILTIN_COUNT);
		store.set(tags::EVENT_ID, get_id());
		store.set(tags::EVENT_CLS, 0);
	}

	event::event(int c, const vector<pair<tag, double>>& i) : event::event()
	{
		for(const pair<tag, double>& kv: i)
		{
			emplace_info(kv.first, kv.second);
		}
		set_cls(c);
	}

	event::event(int c) : event::event()
	{
		set_cls(c);
	}

	void event::clone(const event& e)
	{
		int id = get_id();
		store = e.store;
		store.set(tags::EVENT_ID, id);
	}

	void event::shift_times(double time, const vector<tag>& keys)
	{
		double t = get_time();
		set_time(t > time? t - time : 0);
		for(tag k: keys)
		{
			pair<bool, double> value = store.get(k);
			if(value.first)
			{
				store.set(k, value.second > time? value.second - time : 0);
			}
		}
	}

	bool event::is_initialized() const
	{
		return store.has(tags::EVENT_CLS);
	}

	std::string event::to_string() const
	{
		string s = "( ";
		s.append(std::to_string(get_id()));
		s.append("\tclass:\t");
		s.append(std::to_string(get_cls()));
		store.for_each([&s](tag t, double v)
		{
			s.append("\t");
			s.append(tag_registry::name(t));
			s.append(":\t");
			s.append(std::to_string(v));
		});
		s.append(")");
		return s;
	}

}
