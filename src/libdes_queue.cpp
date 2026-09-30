#include "libdes_queue.hpp"

namespace des{
    queue::queue(unsigned int positions, shared_ptr<policy> pol) : object(id_gen++),
        pos(positions),
        p(pol),
        jobs()
    {
        if(p == nullptr)
        {
            throw invalid_argument("des::queue requires a policy");
        }
        jobs = p -> make_store();
    }

    queue::queue(int positions, shared_ptr<policy> pol) : queue(static_cast<unsigned int>(positions), pol)
    {}

    queue::queue(shared_ptr<policy> pol) : queue(numeric_limits<unsigned int>::max(),  pol)
    {}

    bool queue::enqueue(const shared_ptr<event>& e, double time)
    {
        if(!p -> admit(*e, *jobs, pos))
        {
            return false;
        }
        jobs -> push(e, time);
        ++held;
        return true;
    }

    shared_ptr<event> queue::dequeue()
    {
        if(held == 0)
        {
            throw runtime_error("des::queue trying to dequeue from an empty queue");
        }
        double t = min_time();
        --held;
        return jobs -> pop(t);
    }

    shared_ptr<event> queue::dequeue(double time)
    {
        if(held == 0)
        {
            throw runtime_error("des::queue trying to dequeue from an empty queue");
        }
        --held;
        return jobs -> pop(time);
    }

    shared_ptr<event> queue::dequeue_next(const function<bool(const event&)>& eligible, double time)
    {
        shared_ptr<event> e = jobs -> pop_first(eligible, time);
        if(e != nullptr)
        {
            --held;
        }
        return e;
    }

    bool queue::has_next(const function<bool(const event&)>& eligible) const
    {
        return jobs -> has(eligible);
    }

    void queue::reset(double time, vector<tag> keys, bool)
    {
        jobs -> for_each([time, &keys](event& e){ e.shift_times(time, keys); });
    }

    void queue::clear()
    {
        jobs -> clear();
        held = 0;
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
        if(jobs -> size() != 0)
        {
            throw logic_error("des::queue::set_policy called on a non-empty queue");
        }
        p = shared_ptr<policy>(pol);
        jobs = p -> make_store();
    }

    string queue::to_string() const{
        string s = "\tdes::queue (" + std::to_string(get_id()) + ")\n" + p -> to_string() +"\n\tPositions: " + std::to_string(pos);
        s += " (available: " + std::to_string(pos - in_queue()) + ", in use: " + std::to_string(in_queue()) + ")";
        return s;
    }
}
