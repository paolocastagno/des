#include "libdes_ps.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace des {

ps::ps() : policy("des::ps"), w()
{}

ps::ps(vector<double> weights) : policy("des::ps"), w(std::move(weights))
{
    for(double x : w)
    {
        if(!(x > 0.0) || !std::isfinite(x))
            throw invalid_argument("des::ps: weights must be positive finite numbers");
    }
}

unique_ptr<job_store> ps::make_store() const
{
    return make_unique<ps_store>(w);
}

ps_store::ps_store(vector<double> weights) : w(std::move(weights))
{}

double ps_store::weight(int cls) const
{
    if(cls >= 0 && static_cast<size_t>(cls) < w.size())
        return w[cls];
    return 1.0;
}

void ps_store::advance(double now)
{
    // V grows at rate 1/W while jobs are held, and stands still otherwise
    if(total > 0.0)
        vtime += (now - anchor) / total;
    anchor = now;
}

void ps_store::count(int cls, long delta)
{
    size_t c = static_cast<size_t>(cls);
    if(c >= in_class.size())
        in_class.resize(c + 1, 0);
    in_class[c] += delta;
    // Recomputed from integer counts, so W never drifts
    total = 0.0;
    for(size_t k = 0; k < in_class.size(); k++)
        total += in_class[k] * weight(static_cast<int>(k));
}

void ps_store::schedule_next()
{
    if(!jobs.empty())
        jobs.front().job->set_time(departure(jobs.front()));
}

void ps_store::push(const shared_ptr<event>& e, double now)
{
    advance(now);
    int cls = e->get_cls();
    // The node sets the time of an arriving job to now + its service requirement
    double service = e->get_time() - now;
    jobs.push_back(entry{vtime + service / weight(cls), seq++, e});
    push_heap(jobs.begin(), jobs.end(), after());
    count(cls, +1);
    schedule_next();
}

const shared_ptr<event>& ps_store::next() const
{
    return jobs.front().job;
}

void ps_store::departed(const shared_ptr<event>& e, double now)
{
    count(e->get_cls(), -1);
    e->set_time(now);
    if(jobs.empty())
    {
        // Busy period over: restart the virtual clock, so it never grows large
        vtime = 0.0;
        anchor = now;
        seq = 0;
        return;
    }
    schedule_next();
}

shared_ptr<event> ps_store::pop(double now)
{
    advance(now);
    pop_heap(jobs.begin(), jobs.end(), after());
    shared_ptr<event> e = std::move(jobs.back().job);
    jobs.pop_back();
    departed(e, now);
    return e;
}

shared_ptr<event> ps_store::pop_first(const function<bool(const event&)>& eligible, double now)
{
    advance(now);
    // The heap is not sorted: look for the eligible job departing first among all of them
    auto best = jobs.end();
    for(auto it = jobs.begin(); it != jobs.end(); ++it)
    {
        if(eligible(*it->job) && (best == jobs.end() || after()(*best, *it)))
            best = it;
    }
    if(best == jobs.end())
        return nullptr;
    shared_ptr<event> e = std::move(best->job);
    jobs.erase(best);
    make_heap(jobs.begin(), jobs.end(), after());
    departed(e, now);
    return e;
}

bool ps_store::has(const function<bool(const event&)>& eligible) const
{
    return any_of(jobs.begin(), jobs.end(), [&eligible](const entry& en){ return eligible(*en.job); });
}

size_t ps_store::size() const
{
    return jobs.size();
}

void ps_store::for_each(const function<void(event&)>& f)
{
    for(entry& en : jobs)
        en.job->set_time(departure(en));
    for(entry& en : jobs)
        f(*en.job);
    if(jobs.empty())
        return;
    // Resume from the new times: V = 0 at the earliest one, and each tag such that the
    // job departs at its new time while W stays the same
    double t0 = jobs.front().job->get_time();
    for(const entry& en : jobs)
        t0 = min(t0, en.job->get_time());
    vtime = 0.0;
    anchor = t0;
    for(entry& en : jobs)
        en.finish = (en.job->get_time() - t0) / total;
    make_heap(jobs.begin(), jobs.end(), after());
    schedule_next();
}

void ps_store::clear()
{
    jobs.clear();
    in_class.assign(in_class.size(), 0);
    total = 0.0;
    vtime = 0.0;
    anchor = 0.0;
    seq = 0;
}

} // namespace des
