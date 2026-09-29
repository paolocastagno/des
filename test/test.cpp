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

#ifndef DES_TEST_EVENTS
#define DES_TEST_EVENTS 100000
#endif

#ifndef DES_TEST_RUNS
#define DES_TEST_RUNS 5
#endif

    const int n_events = DES_TEST_EVENTS;
    const int n_runs   = DES_TEST_RUNS;
    int run = 0;

    do {
        double sim_time = 0.0;
        for (int i = 0; i < n_events; ++i)
        {
            e = net.next_event();
            sim_time = e->get_time();
            net.route(e);
        }

        double mean_soj = sojourn->mean(0);

        cout << "Run " << run
             << "  sim_time = " << sim_time
             << "  mean sojourn = " << mean_soj << "\n";

        // net.reset propagates reset(true) to all node-attached observers,
        // storing each run's result for cross-run confidence intervals.
        net.reset(sim_time, {}, true);
    }
    while (++run < n_runs);

    // ── Cross-run confidence intervals (alpha = 0.05) ──────────────────────
    auto [thr_lo, thr_hi] = net.get_flow_ci(1, 2, 0, 0.05);
    auto [soj_lo, soj_hi] = sojourn->confidence_interval(0.05, 0);

    cout << "\nResults over " << n_runs << " replications (95% CI):\n"
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
    auto gen2  = make_shared<mt19937_64>();
    gen2->seed(137);

    auto src2 = make_shared<des::source>(vector<double>{lambda2}, "Source2", gen2);

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
        "M/M/2",
        gen2);

    auto snk2 = make_shared<des::sink>("Sink2");

    // ── Observer ───────────────────────────────────────────────────────────
    auto sojourn2 = make_shared<des::scalar>(NODE_SOJOURN, 1);
    sta2->attach(SIGNAL_NODE_DEPARTURE, sojourn2);

    // ── Network ────────────────────────────────────────────────────────────
    vector<vector<vector<double>>> routing2 = {
        {{0}, {1}, {0}},   // source  → station
        {{0}, {0}, {1}},   // station → sink
        {{0}, {0}, {0}}    // sink    → nowhere
    };

    vector<shared_ptr<des::node>> nodes2{src2, sta2, snk2};
    des::network net2(nodes2, routing2, gen2);

    // ── Bootstrap ─────────────────────────────────────────────────────────
    auto e2 = make_shared<des::event>();
    e2->set_cls(0);
    e2->set_time(0.0);
    e2->set_info(EVENT_NODE, 0);
    nodes2.at(0)->arrival(e2);
    nodes2.clear();

    // ── Simulation loop ────────────────────────────────────────────────────
    cout << "\n\nM/M/2/∞ open queueing system\n"
         << "  lambda = " << lambda2
         << "  mu = "     << mu2
         << "  c = "      << c2
         << "  rho = "    << rho2 << "\n"
         << "  Erlang-C  C(2,a) = " << erlang_c2 << "\n"
         << "  Theoretical mean sojourn = " << W2_theory << "\n\n";

    int run2 = 0;
    do {
        double sim_time2 = 0.0;
        for (int i = 0; i < n_events; ++i)
        {
            e2 = net2.next_event();
            sim_time2 = e2->get_time();
            net2.route(e2);
        }

        double mean_soj2 = sojourn2->mean(0);

        cout << "Run " << run2
             << "  sim_time = " << sim_time2
             << "  mean sojourn = " << mean_soj2 << "\n";

        net2.reset(sim_time2, {}, true);
    }
    while (++run2 < n_runs);

    // ── Cross-run confidence intervals (alpha = 0.05) ──────────────────────
    auto [thr2_lo, thr2_hi] = net2.get_flow_ci(1, 2, 0, 0.05);
    auto [soj2_lo, soj2_hi] = sojourn2->confidence_interval(0.05, 0);

    cout << "\nResults over " << n_runs << " replications (95% CI):\n"
         << "  Throughput   [" << thr2_lo << ", " << thr2_hi << "]"
         << "  (theory: " << lambda2 << ")\n"
         << "  Mean sojourn [" << soj2_lo << ", " << soj2_hi << "]"
         << "  (theory: " << W2_theory << ")\n";

    return 0;
}
