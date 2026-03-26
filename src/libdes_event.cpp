#include "libdes_event.hpp"

namespace des
{
	event::event() : object(id_gen++),
			info()
	{
		set_info(EVENT_ID, get_id());
		set_info(EVENT_CLS, 0);
	}

	event::event(int c, unordered_map<string,double> i) : event::event()
	{
		info.insert(i.begin(), i.end());
		set_cls(c);
	}

	event::event(int c) : event::event()
	{
		set_cls(c);
	}

	void event::clone(event e)
	{
		int id = get_id();
		info = e.info;
		emplace_info(EVENT_ID, id);
	}

	void event::emplace_info(string name, double val)
	{
		if(EVENT_TIME== name || EVENT_CONSTRAINT== name)
		{
			throw invalid_argument("Trying to emplace protected key with event::emplace_info(). If it is not a mistake, use event::set_*(double& time) instead. Otherwize, check des/util/util_const.hpp for reserved keys");
		}
		info[name] = val;
	}
	
	void event::remove_info(string name)
	{
		if(EVENT_TIME== name || EVENT_CONSTRAINT== name)
		{
			throw invalid_argument("Trying to remove protected key with event::emplace_info(). If it is not a mistake, use event::set_*(double& time) instead. Otherwize, check des/util/util_const.hpp for reserved keys");
		}
		info.erase(name);
	}

	bool event::is_initialized() const
	{
		return info.find(EVENT_CLS) != info.end();
	}
	
	std::string event::to_string() const
	{
		string s = "( ";
		s.append(std::to_string(get_id()));
		s.append("\tclass:\t");
		s.append(std::to_string(get_cls()));
		for(auto it = info.begin(); it != info.end(); it++)
		{
			s.append("\t");
			s.append(it -> first);
			s.append(":\t");
			s.append(std::to_string(it -> second));
		}
		s.append(")");
		return s;
	}
	
}