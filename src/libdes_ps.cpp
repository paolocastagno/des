#include "libdes_ps.hpp"

namespace des {

ps::ps() : policy("des::ps"), w()
{}

ps::ps(vector<double> weights) : policy("des::ps"), w(std::move(weights))
{}

double ps::get_weight(unsigned int cls) const
{
    if(cls < w.size())
        return w[cls];
    return 1.0;
}

double ps::total_weight(const list<shared_ptr<event>>& l) const
{
    double W = 0.0;
    for(const auto& ev : l)
        W += get_weight(static_cast<unsigned int>(ev->get_cls()));
    return W;
}

bool ps::update(shared_ptr<event> e, list<shared_ptr<event>>& l, unsigned int positions, double time)
{
    if(l.size() >= positions)
        return false;

    double W     = total_weight(l);           // total weight before arrival
    double w_new = get_weight(static_cast<unsigned int>(e->get_cls()));
    double W_new = W + w_new;

    // Rescale existing jobs: d_j' = time + (d_j - time) * W_new / W
    // The scale factor W_new/W is class-independent → sorted order preserved.
    if(W > 0.0)
    {
        double scale = W_new / W;
        for(auto& ev : l)
            ev->set_time(time + (ev->get_time() - time) * scale);
    }

    // The station sets e->get_time() = arrival_time + service_time_alone, so
    // the equivalent remaining time for the new job is its raw service time.
    double r_new = e->get_time() - time;
    e->set_time(time + r_new * W_new / w_new);

    return true;
}

bool ps::update(shared_ptr<event>, list<shared_ptr<event>>&, unsigned int)
{
    throw runtime_error("des::ps::update wrong function call: time parameter required for processor sharing policy");
    return false;
}

bool ps::update(shared_ptr<event>, list<shared_ptr<event>>&, double)
{
    throw runtime_error("des::ps::update wrong function call: no rate parameter in ps class");
    return false;
}

bool ps::front()
{
    return true;
}

void ps::on_dequeue(list<shared_ptr<event>>& l, double time)
{
    if(l.size() <= 1)
        return; // nothing remains after this departure

    // The departing job is the front element.
    double W     = total_weight(l);           // total weight before departure
    double w_dep = get_weight(static_cast<unsigned int>(l.front()->get_cls()));
    double W_new = W - w_dep;

    if(W_new <= 0.0)
        return;

    // Rescale remaining jobs (skip front — that is the departing one):
    // d_j' = time + (d_j - time) * W_new / W
    double scale = W_new / W;
    auto it = l.begin();
    ++it; // skip departing job
    for(; it != l.end(); ++it)
        (*it)->set_time(time + ((*it)->get_time() - time) * scale);
}

} // namespace des
