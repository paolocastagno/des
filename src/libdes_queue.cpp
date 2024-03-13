#include "libdes_queue.hpp"

namespace des{
    queue::queue(unsigned int positions, shared_ptr<policy> pol) : object(id_gen++), 
        pos(positions),
        lst(),
        p(pol)
    {}

    queue::queue(int positions, shared_ptr<policy> pol) : object(id_gen++),
        pos(static_cast<unsigned int>(positions)),
        lst(),
        p(pol)
    {}

    queue::queue(shared_ptr<policy> pol) : queue(numeric_limits<unsigned int>::max(),  pol)
    {}

    void queue::insert(shared_ptr<event> e)
    {
        if(lst.empty() || *(e.get()) < *(lst.front().get()))
        {
            lst.insert(lst.begin(), e);
        }
        else if(*(e.get()) >= *(lst.back().get()))
        {
            lst.insert(lst.end(), e);
        }
        else
        {
            list<shared_ptr<event>>::iterator it = lst.begin();
            while(it !=lst.end() && *(it -> get()) < *(e.get()))
            {
                ++it;
            }
            lst.insert(it, e);
        }
    }

    bool queue::enqueue(shared_ptr<event> e, double time)
    {
        bool ret = false;
        if(p == nullptr)
        {
            throw runtime_error("des::queue trying to enqueue with empty policy");
        }
        // Update events' order in the queue
        if(p -> update(e, lst, pos, time))
        {
            // Create a new list and merge the new element into the list 
            insert(e);
            ret = true;
        }
        return ret;
    }

    shared_ptr<event> queue::dequeue()
    {
        if(lst.size() == 0)
        {
            throw runtime_error("des::queue trying to dequeue from an empty queue");
        }
        shared_ptr<event> ret;
        if(p -> front())
        {
            ret = lst.front();
            lst.pop_front();
        }
        else
        {
            ret = lst.back();
            lst.pop_back();
        }
        return ret;
    }

    double queue::min_time() const
    {
        if(lst.size() > 0)
            return lst.front() -> get_time();
        else
            return __DBL_MAX__;
    }

    bool queue::is_full() const
    {
        return lst.size() < pos? false : true;
    }

    int queue::in_queue() const
    {
        return lst.size();
    }

    void queue::reset(double time, vector<string> keys, bool newrun)
    {
        for(auto it = lst.begin(); it != lst.end(); it++)
        {
            (*it) -> reset(time, keys);
        }
    }

    void queue::clear()
    {
        lst.clear();
    }

    int queue::get_positions() const
    {
        return pos;
    }

    void queue::set_positions(int positions)
    {
        pos = (unsigned int)positions;
    }

    string queue::get_policy() const
    {
        return p -> get_description();
    }

    void queue::set_policy(policy *pol)
    {
        p = shared_ptr<policy>(pol);
    }

    string queue::to_string() const{
        string s = "\tdes::queue (" + std::to_string(get_id()) + ")\n" + p -> to_string() +"\n\tPositions: " + std::to_string(pos);
        s += " (available: " + std::to_string(pos - in_queue()) + ", in use: " + std::to_string(in_queue()) + ")";
        return s;
    }
}