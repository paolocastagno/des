#include "libdes_tag.hpp"

#include <deque>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace des
{
	namespace
	{
		// Names of the reserved tags, in identifier order. They must match the string
		// constants in libdes_const.hpp; literals are used so that the registry can be
		// built during static initialisation of any translation unit.
		const char* const builtin_names[tags::BUILTIN_COUNT] = {
			"id",                   // EVENT_ID
			"class",                // EVENT_CLS
			"time",                 // EVENT_TIME
			"constraint",           // EVENT_CONSTRAINT
			"reroute",              // EVENT_REROUTE
			"node",                 // EVENT_NODE
			"queue_idx",            // EVENT_QUEUE
			"server_idx",           // EVENT_SERVER
			"reject",               // EVENT_REJECT
			"arrival_time",         // NODE_ARRIVAL
			"service_start_time",   // NODE_SERVICE_START
			"node_sojourn",         // NODE_SOJOURN
			"node_wait",            // NODE_WAIT
			"node_service"          // NODE_SERVICE
		};

		struct registry_state
		{
			shared_mutex mtx;
			unordered_map<string, unsigned int> ids;
			// deque keeps references to existing names valid while new ones are added
			deque<string> names;

			registry_state()
			{
				for(unsigned int i = 0; i < tags::BUILTIN_COUNT; i++)
				{
					ids.emplace(builtin_names[i], i);
					names.emplace_back(builtin_names[i]);
				}
			}
		};

		registry_state& state()
		{
			static registry_state s;
			return s;
		}
	}

	namespace
	{
		// Returns the tag of name, creating it on first use
		tag find_or_add(const string& name)
		{
			registry_state& s = state();
			{
				shared_lock<shared_mutex> lock(s.mtx);
				auto it = s.ids.find(name);
				if(it != s.ids.end())
				{
					return tag{it -> second};
				}
			}
			unique_lock<shared_mutex> lock(s.mtx);
			// Another thread may have added the name in the meantime
			auto it = s.ids.emplace(name, static_cast<unsigned int>(s.names.size()));
			if(it.second)
			{
				s.names.push_back(name);
			}
			return tag{it.first -> second};
		}
	}

	tag tag_registry::define(const string& name)
	{
		tag t = find_or_add(name);
		if(is_builtin(t))
		{
			throw invalid_argument("des::tag_registry::define: \"" + name + "\" is reserved by the library, use the predefined des::tags instead");
		}
		return t;
	}

	pair<bool, tag> tag_registry::find(const string& name)
	{
		registry_state& s = state();
		shared_lock<shared_mutex> lock(s.mtx);
		auto it = s.ids.find(name);
		if(it == s.ids.end())
		{
			return pair<bool, tag>(false, tag{0});
		}
		return pair<bool, tag>(true, tag{it -> second});
	}

	string tag_registry::name(tag t)
	{
		registry_state& s = state();
		shared_lock<shared_mutex> lock(s.mtx);
		return s.names.at(t.id);
	}

	unsigned int tag_registry::size()
	{
		registry_state& s = state();
		shared_lock<shared_mutex> lock(s.mtx);
		return static_cast<unsigned int>(s.names.size());
	}
}
