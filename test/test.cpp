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
#include <libdes_scalar.hpp>

using namespace std;

int main()
{
    // ── Random number generator ────────────────────────────────────────────
    auto gen = make_shared<mt19937_64>();
    gen->seed(42);

    // ── M/M/1 parameters ──────────────────────────────────────────────────
    const double lambda = 0.8;   // arrival rate
    const double mu     = 1.0;   // service rate  →  rho = 0.8

    // ── Nodes ──────────────────────────────────────────────────────────────
    auto src = make_shared<des::source>(vector<double>{lambda}, "Source", gen);

    auto svc = make_shared<exponential_distribution<double>>(mu);
    auto sta = make_shared<des::station<double, exponential_distribution>>(
        vector<vector<shared_ptr<exponential_distribution<double>>>>{{{svc}}},
        1,        // 1 server
        1,        // 1 job per server
        1,        // 1 event class
        INT_MAX,  // unlimited waiting queue
        "M/M/1",
        gen);

    auto snk = make_shared<des::sink>("Sink");

    // ── Observer ───────────────────────────────────────────────────────────
    // Attaching a scalar to SIGNAL_NODE_DEPARTURE lets us measure the
    // sojourn time (NODE_SOJOURN = wait + service) at the station.
    // The scalar description must match the message key to extract.
    auto sojourn = make_shared<des::scalar>(NODE_SOJOURN, 1);
    sta->attach(SIGNAL_NODE_DEPARTURE, sojourn);

    // ── Network ────────────────────────────────────────────────────────────
    // Node indices:  source = 0   station = 1   sink = 2
    vector<vector<vector<double>>> routing = {
        {{0}, {1}, {0}},   // source  → station (p = 1)
        {{0}, {0}, {1}},   // station → sink    (p = 1)
        {{0}, {0}, {0}}    // sink    → nowhere
    };

    vector<shared_ptr<des::node>> nodes{src, sta, snk};
    des::network net(nodes, routing, gen);

    // ── Bootstrap ─────────────────────────────────────────────────────────
    // Inject the first event directly into the source so it schedules
    // the first arrival; after this the simulation is self-sustaining.
    auto e = make_shared<des::event>();
    e->set_cls(0);
    e->set_time(0.0);
    e->set_info(EVENT_NODE, 0);
    nodes.at(0)->arrival(e);
    nodes.clear();

    // ── Simulation loop ────────────────────────────────────────────────────
    cout << "M/M/1 open queueing system\n"
         << "  lambda = " << lambda
         << "  mu = "     << mu
         << "  rho = "    << lambda / mu << "\n"
         << "  Theoretical mean sojourn = " << 1.0 / (mu - lambda) << "\n\n";

    const int N_EVENTS = 100000;
    const int N_RUNS   = 5;
    int run = 0;

    do {
        double sim_time = 0.0;
        for (int i = 0; i < N_EVENTS; ++i)
        {
            e = net.next_event();
            sim_time = e->get_time();
            net.route(e, sim_time);
        }

        // sojourn->run_avg(0) is the sample mean sojourn for this run.
        // Calling stddev(0) once updates the internal cross-run mean (s)
        // so that the subsequent net.reset() can store it for CI computation.
        double mean_soj = sojourn->run_avg(0);
        sojourn->stddev(0);

        cout << "Run " << run
             << "  sim_time = " << sim_time
             << "  mean sojourn = " << mean_soj << "\n";

        // net.reset propagates reset(true) to all node-attached observers,
        // storing each run's result for cross-run confidence intervals.
        net.reset(sim_time, {}, true);
    }
    while (++run < N_RUNS);

    // ── Cross-run confidence intervals (alpha = 0.05) ──────────────────────
    auto [thr_lo, thr_hi] = net.get_flow_ci(1, 2, 0, 0.05);
    auto [soj_lo, soj_hi] = sojourn->confidence_interval(0.05, 0);

    cout << "\nResults over " << N_RUNS << " replications (95% CI):\n"
         << "  Throughput   [" << thr_lo << ", " << thr_hi << "]"
         << "  (theory: " << lambda << ")\n"
         << "  Mean sojourn [" << soj_lo << ", " << soj_hi << "]"
         << "  (theory: " << 1.0 / (mu - lambda) << ")\n";

    return 0;
}
