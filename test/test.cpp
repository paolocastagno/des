#include <string>
#include <iostream>
#include <memory>
#include <random>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_station.hpp>
#include <libdes_source.hpp>
#include <libdes_sink.hpp>
#include <libdes_network.hpp>
#include <libdes_ratio.hpp>
#include <libdes_scalar.hpp>

using namespace std;

// ── Stopping rules ─────────────────────────────────────────────────────────
#ifndef DES_TEST_TOLERANCE
#define DES_TEST_TOLERANCE 0.01
#endif

#ifndef DES_TEST_RUN_TOLERANCE
#define DES_TEST_RUN_TOLERANCE 0.05
#endif

#ifndef DES_TEST_MIN_RUNS
#define DES_TEST_MIN_RUNS 10
#endif

#ifndef DES_TEST_MIN_CYCLES
#define DES_TEST_MIN_CYCLES 30
#endif

const double alpha         = 0.05;                     // 95% confidence intervals
const double tolerance     = DES_TEST_TOLERANCE;       // across runs: stop when the throughput CI is within this fraction of its mean
const double run_tolerance = DES_TEST_RUN_TOLERANCE;   // within a run: end it when its own throughput CI is within this fraction
const size_t min_runs      = DES_TEST_MIN_RUNS;        // runs before the first check across runs
const long   min_cycles    = DES_TEST_MIN_CYCLES;      // cycles before the first check within a run

// An interval is narrow enough when its half-width is within tol times its midpoint
bool narrow(pair<double, double> ci, double tol)
{
    return ci.second - ci.first <= tol * (ci.first + ci.second);
}

struct replications
{
    long   cycles   = 0;     // regeneration cycles over all runs
    long   jobs     = 0;     // jobs served by the station over all runs
    double sim_time = 0.0;   // simulated time over all runs
};

// Runs replications of a source (node 0) -> station (node 1) -> sink (node 2)
// network with two stopping rules. A regeneration point is a departure that
// leaves the station empty: arrivals are Poisson, so the time to the next one is
// memoryless and the model is back in its state at time 0.
//  - Within a run, each regeneration cycle gives `throughput` one pair: the jobs
//    the station served in the cycle and the cycle's length. The cycles are
//    independent, so the ratio observer gives the run's own throughput CI; the run
//    ends at the first regeneration point at which that CI is within
//    `run_tolerance` of the estimate.
//  - Every run starts and ends at a regeneration point, so the runs are
//    independent and identically distributed, with no warm-up needed. Runs are
//    added until the throughput CI across runs is within `tolerance` of its mean.
replications replicate(des::network& net, des::node& sta, des::ratio& throughput)
{
    replications r;
    do {
        double sim_time = 0.0, start = 0.0;   // now, and the start of the current cycle
        long jobs = 0;                        // jobs served in the current cycle
        bool done = false;
        while (!done)
        {
            auto e = net.next_event();        // find the globally earliest event
            sim_time = e->get_time();
            bool from_station = e->get_info(EVENT_NODE).second == 1;   // read it before route() moves e
            net.route(e);                     // route it → fires observer notifications
            if (from_station)
            {
                ++jobs;
                if (sta.queue_length() + sta.service_length() == 0)   // regeneration point: close the cycle
                {
                    throughput.update(jobs, sim_time - start, 0);
                    ++r.cycles;
                    r.jobs += jobs;
                    jobs = 0;
                    start = sim_time;
                    done = throughput.n_updates(0) >= min_cycles
                        && narrow(throughput.run_confidence_interval(alpha, 0), run_tolerance);
                }
            }
        }

        // net.reset propagates reset(true) to all node-attached observers,
        // storing each run's result for cross-run confidence intervals.
        net.reset(sim_time, {}, true);   // close the run at the regeneration point
        throughput.reset(true);          // store the run's throughput
        r.sim_time += sim_time;
    }
    while (throughput.completed_runs(0) < min_runs
           || !narrow(throughput.confidence_interval(alpha, 0), tolerance));
    return r;
}

int main()
{
    // ── M/M/1 parameters ──────────────────────────────────────────────────
    const double lambda = 0.8;   // arrival rate
    const double mu     = 1.0;   // service rate  →  rho = 0.8

    // ── Nodes ──────────────────────────────────────────────────────────────
    auto arr = make_shared<exponential_distribution<double>>(lambda);
    auto src = make_shared<des::source<double, exponential_distribution>>(
        vector<shared_ptr<exponential_distribution<double>>>{arr}, "Source");

    auto svc = make_shared<exponential_distribution<double>>(mu);
    auto sta = make_shared<des::station<double, exponential_distribution>>(
        vector<vector<shared_ptr<exponential_distribution<double>>>>{{{svc}}},
        1,        // 1 server
        1,        // 1 job per server
        1,        // 1 waiting queue
        INT_MAX,  // unlimited waiting queue
        "M/M/1");

    auto snk = make_shared<des::sink>("Sink");

    // ── Observer ───────────────────────────────────────────────────────────
    // Attaching a scalar to SIGNAL_NODE_DEPARTURE lets us measure the
    // sojourn time (NODE_SOJOURN = wait + service) at the station.
    // The scalar description must match the message key to extract.
    auto sojourn = make_shared<des::scalar>(NODE_SOJOURN, 1);
    sta->attach(SIGNAL_NODE_DEPARTURE, sojourn);

    // The station's throughput is measured over regeneration cycles:
    // jobs served in a cycle / cycle length (see replicate()).
    des::ratio throughput("throughput", 1);

    // ── Network ────────────────────────────────────────────────────────────
    // Node indices:  source = 0   station = 1   sink = 2
    // The network gives every node its own random streams, all derived from the seed (42)
    vector<vector<vector<double>>> routing = {
        {{0}, {1}, {0}},   // source  → station (p = 1)
        {{0}, {0}, {1}},   // station → sink    (p = 1)
        {{0}, {0}, {0}}    // sink    → nowhere
    };

    vector<shared_ptr<des::node>> nodes{src, sta, snk};
    des::network net(nodes, routing, 42);

    // ── Bootstrap ─────────────────────────────────────────────────────────
    // Inject the first event directly into the source so it schedules
    // the first arrival; after this the simulation is self-sustaining.
    auto e = make_shared<des::event>();
    e->set_cls(0);
    e->set_time(0.0);
    e->set_info(EVENT_NODE, 0);
    nodes.at(0)->arrival(e);
    nodes.clear();

    // ── Simulation ─────────────────────────────────────────────────────────
    cout << "M/M/1 open queueing system\n"
         << "  lambda = " << lambda
         << "  mu = "     << mu
         << "  rho = "    << lambda / mu << "\n"
         << "  Theoretical mean sojourn = " << 1.0 / (mu - lambda) << "\n\n";

    replications r = replicate(net, *sta, throughput);

    // ── Cross-run confidence intervals (alpha = 0.05) ──────────────────────
    auto [thr_lo, thr_hi] = throughput.confidence_interval(alpha, 0);
    auto [soj_lo, soj_hi] = sojourn->confidence_interval(alpha, 0);

    cout << "Results over " << throughput.completed_runs(0) << " replications (95% CI):\n"
         << "  " << r.jobs << " jobs served in " << r.cycles << " regeneration cycles, simulated time " << r.sim_time << "\n"
         << "  Throughput   [" << thr_lo << ", " << thr_hi << "]"
         << "  (theory: " << lambda << ")\n"
         << "  Mean sojourn [" << soj_lo << ", " << soj_hi << "]"
         << "  (theory: " << 1.0 / (mu - lambda) << ")\n";

    // ══════════════════════════════════════════════════════════════════════
    // M/M/2/∞  open queueing system
    // Erlang-C formula:
    //   a   = λ/μ            (total offered load in Erlangs)
    //   ρ   = λ/(c·μ)        (per-server utilisation; ρ < 1 for stability)
    //   P₀  = 1 / [Σ_{n=0}^{c-1} aⁿ/n!  +  aᶜ / (c!·(1−ρ))]
    //   C   = [aᶜ / (c!·(1−ρ))] · P₀     (Erlang-C, = P{waiting > 0})
    //   W   = C / (c·μ − λ) + 1/μ        (mean sojourn, Little's law)
    // ══════════════════════════════════════════════════════════════════════

    // ── M/M/2 parameters ──────────────────────────────────────────────────
    const double lambda2 = 1.6;   // arrival rate
    const double mu2     = 1.0;   // service rate per server  →  rho = 0.8
    const int    c2      = 2;

    const double a2        = lambda2 / mu2;
    const double rho2      = a2 / c2;
    // P0 for c=2: 1 / (1 + a + a²/(2·(1−ρ)))
    const double erlang_c2_term = (a2 * a2) / (2.0 * (1.0 - rho2));
    const double P0_2           = 1.0 / (1.0 + a2 + erlang_c2_term);
    const double erlang_c2      = erlang_c2_term * P0_2;
    const double W2_theory      = erlang_c2 / (c2 * mu2 - lambda2) + 1.0 / mu2;

    // ── Nodes ──────────────────────────────────────────────────────────────
    auto arr2 = make_shared<exponential_distribution<double>>(lambda2);
    auto src2 = make_shared<des::source<double, exponential_distribution>>(
        vector<shared_ptr<exponential_distribution<double>>>{arr2}, "Source2");

    auto svc2 = make_shared<exponential_distribution<double>>(mu2);
    auto sta2 = make_shared<des::station<double, exponential_distribution>>(
        vector<vector<shared_ptr<exponential_distribution<double>>>>{
            {svc2},   // server 0, class 0
            {svc2}    // server 1, class 0
        },
        2,        // 2 servers
        1,        // 1 job per server
        1,        // 1 waiting queue
        INT_MAX,  // unlimited capacity
        "M/M/2");

    auto snk2 = make_shared<des::sink>("Sink2");

    // ── Observer ───────────────────────────────────────────────────────────
    auto sojourn2 = make_shared<des::scalar>(NODE_SOJOURN, 1);
    sta2->attach(SIGNAL_NODE_DEPARTURE, sojourn2);
    des::ratio throughput2("throughput", 1);

    // ── Network ────────────────────────────────────────────────────────────
    vector<vector<vector<double>>> routing2 = {
        {{0}, {1}, {0}},   // source  → station
        {{0}, {0}, {1}},   // station → sink
        {{0}, {0}, {0}}    // sink    → nowhere
    };

    vector<shared_ptr<des::node>> nodes2{src2, sta2, snk2};
    des::network net2(nodes2, routing2, 137);

    // ── Bootstrap ─────────────────────────────────────────────────────────
    auto e2 = make_shared<des::event>();
    e2->set_cls(0);
    e2->set_time(0.0);
    e2->set_info(EVENT_NODE, 0);
    nodes2.at(0)->arrival(e2);
    nodes2.clear();

    // ── Simulation ─────────────────────────────────────────────────────────
    // A departure that leaves both servers idle is a regeneration point too.
    cout << "\n\nM/M/2/∞ open queueing system\n"
         << "  lambda = " << lambda2
         << "  mu = "     << mu2
         << "  c = "      << c2
         << "  rho = "    << rho2 << "\n"
         << "  Erlang-C  C(2,a) = " << erlang_c2 << "\n"
         << "  Theoretical mean sojourn = " << W2_theory << "\n\n";

    replications r2 = replicate(net2, *sta2, throughput2);

    // ── Cross-run confidence intervals (alpha = 0.05) ──────────────────────
    auto [thr2_lo, thr2_hi] = throughput2.confidence_interval(alpha, 0);
    auto [soj2_lo, soj2_hi] = sojourn2->confidence_interval(alpha, 0);

    cout << "Results over " << throughput2.completed_runs(0) << " replications (95% CI):\n"
         << "  " << r2.jobs << " jobs served in " << r2.cycles << " regeneration cycles, simulated time " << r2.sim_time << "\n"
         << "  Throughput   [" << thr2_lo << ", " << thr2_hi << "]"
         << "  (theory: " << lambda2 << ")\n"
         << "  Mean sojourn [" << soj2_lo << ", " << soj2_hi << "]"
         << "  (theory: " << W2_theory << ")\n";

    return 0;
}
