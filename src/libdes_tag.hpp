#ifndef TAG_H
#define TAG_H

#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace des
{
	/**
	 * @brief Compact identifier of an event/message information field.
	 *
	 * Tags are obtained once, at setup time, from des::tag_registry (or are one of the
	 * predefined des::tags), and then used to read and write fields without hashing strings.
	 */
	struct tag
	{
		unsigned int id;
	};

	constexpr bool operator==(tag a, tag b) { return a.id == b.id; }
	constexpr bool operator!=(tag a, tag b) { return a.id != b.id; }

	/**
	 * @brief Tags of the fields reserved by the library. Their names are the string
	 * constants with the same identifier in libdes_const.hpp (e.g. tags::EVENT_TIME is "time").
	 */
	namespace tags
	{
		inline constexpr tag EVENT_ID{0};
		inline constexpr tag EVENT_CLS{1};
		inline constexpr tag EVENT_TIME{2};
		inline constexpr tag EVENT_CONSTRAINT{3};
		inline constexpr tag EVENT_REROUTE{4};
		inline constexpr tag EVENT_NODE{5};
		inline constexpr tag EVENT_QUEUE{6};
		inline constexpr tag EVENT_SERVER{7};
		inline constexpr tag EVENT_REJECT{8};
		inline constexpr tag NODE_ARRIVAL{9};
		inline constexpr tag NODE_SERVICE_START{10};
		inline constexpr tag NODE_SOJOURN{11};
		inline constexpr tag NODE_WAIT{12};
		inline constexpr tag NODE_SERVICE{13};
		/**
		 * @brief Number of reserved tags; user-defined tags get identifiers from this value on.
		 */
		inline constexpr unsigned int BUILTIN_COUNT = 14;
	}

	class tag_registry;
	class tag_store;
}

/**
 * @brief Process-wide mapping between field names and tags.
 *
 * Define user fields once, during model setup, and keep the returned tags:
 *
 *     des::tag priority = des::tag_registry::define("priority");
 *     e->emplace_info(priority, 2);
 *
 * All methods are thread-safe.
 */
class des::tag_registry
{
	public:
		/**
		 * @brief Returns the tag of the user field @p name, creating it on first use.
		 *
		 * Defining the same name again returns the same tag.
		 *
		 * @throws invalid_argument if @p name is reserved by the library (see des::tags)
		 */
		static tag define(const string& name);
		/**
		 * @brief Looks up @p name without creating it.
		 *
		 * @return (true, tag) if the name is known, (false, _) otherwise
		 */
		static pair<bool, tag> find(const string& name);
		/**
		 * @brief Returns the name of tag @p t
		 *
		 * @throws out_of_range if @p t was not produced by the registry
		 */
		static string name(tag t);
		/**
		 * @brief Returns the number of tags defined so far, reserved ones included
		 */
		static unsigned int size();
		/**
		 * @brief Tells whether @p t is reserved by the library
		 */
		static constexpr bool is_builtin(tag t)
		{
			return t.id < tags::BUILTIN_COUNT;
		}
};

/**
 * @brief Values of the fields of an event or a message, indexed by tag.
 */
class des::tag_store
{
	public:
		/**
		 * @brief Tells whether the field @p t holds a value
		 */
		inline bool has(tag t) const
		{
			return t.id < slots.size() && slots[t.id].present;
		}
		/**
		 * @brief Returns (true, value) if the field @p t holds a value, (false, 0) otherwise
		 */
		inline pair<bool, double> get(tag t) const
		{
			if(has(t))
			{
				return pair<bool, double>(true, slots[t.id].value);
			}
			return pair<bool, double>(false, 0.0);
		}
		/**
		 * @brief Returns the value of the field @p t, or 0 if it holds none
		 */
		inline double value(tag t) const
		{
			return has(t) ? slots[t.id].value : 0.0;
		}
		/**
		 * @brief Sets the value of the field @p t, replacing any previous one
		 */
		inline void set(tag t, double v)
		{
			if(t.id >= slots.size())
			{
				slots.resize(t.id + 1);
			}
			slots[t.id].value = v;
			slots[t.id].present = true;
		}
		/**
		 * @brief Sets the value of the field @p t only if it holds none
		 *
		 * @return true if the value was set
		 */
		inline bool insert(tag t, double v)
		{
			if(has(t))
			{
				return false;
			}
			set(t, v);
			return true;
		}
		/**
		 * @brief Removes the value of the field @p t, if any
		 */
		inline void remove(tag t)
		{
			if(t.id < slots.size())
			{
				slots[t.id].present = false;
			}
		}
		/**
		 * @brief Removes all values, keeping the allocated storage
		 */
		inline void clear()
		{
			for(slot& s: slots)
			{
				s.present = false;
			}
		}
		/**
		 * @brief Allocates room for the fields with identifier below @p n
		 */
		inline void reserve(unsigned int n)
		{
			if(n > slots.size())
			{
				slots.resize(n);
			}
		}
		/**
		 * @brief Calls @p f(tag, value) for every field holding a value, in tag order
		 */
		template <typename F>
		void for_each(F f) const
		{
			for(unsigned int i = 0; i < slots.size(); i++)
			{
				if(slots[i].present)
				{
					f(tag{i}, slots[i].value);
				}
			}
		}
	private:
		struct slot
		{
			double value = 0.0;
			bool present = false;
		};
		vector<slot> slots;
};

#endif
