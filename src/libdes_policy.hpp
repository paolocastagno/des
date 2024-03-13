#ifndef POLICY_H
#define POLICY_H

#include <string>
#include <list>
#include <memory>

#include "libdes_object.hpp"
#include "libdes_event.hpp"


using namespace std;

namespace des
{
	class policy;
}

class des::policy : public des::object
{
	public:
		policy(string d) : object(id_gen++)
		{
			description = d;
		}
		virtual bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double time)=0;
		virtual bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions)=0;
		virtual bool update(shared_ptr<event> e, list<shared_ptr<event>>& l, double time)=0;
		virtual bool front()=0;
		inline string get_description() const
		{
			return description;
		}

		virtual ~policy(){};

		inline void set_description(string str)
		{
			description = str;
		}

		virtual inline string to_string() const override {
			string s = "\tdes::policy (" + std::to_string(get_id()) + ")\t" + description + "\n";
			return s;
		}
		
		virtual inline void clear() override
		{}

	private:
		string description;
		inline static unsigned int id_gen = 0;
};


#endif