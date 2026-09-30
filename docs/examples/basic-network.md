# Example: M/M/1 Open Queueing System

This example walks through the M/M/1 part of `test/test.cpp`. It builds a minimal open queueing network and measures the sojourn time with an observer. It then estimates throughput and mean sojourn time over independent replications, with two stopping rules built on regeneration points: one ends each run, and one ends the simulation. The M/M/2 part of the file repeats the same steps.

The full source is in [`test/test.cpp`](../../test/test.cpp).

---

## Network Topology

```
Source (node 0)
    |  Poisson arrivals, rate λ = 0.8
    v
M/M/1 Station (node 1) — exponential service, rate μ = 1.0, 1 server, ∞ queue
    |  all jobs
    v
Sink (node 2)
```

**Theoretical results** for ρ = λ/μ = 0.8:
- Throughput = λ = 0.8
- Mean sojourn = 1 / (μ − λ) = 5.0

---

## Step-by-Step Walkthrough

### 1. Random Numbers

No generator is created by hand: the network constructor (step 4) takes the seed, `42`, and gives every node its own random streams. Fixing the seed makes runs reproducible. See [Random streams](../architecture.md#5-random-streams).

---

### 2. Nodes

```cpp
// Source: Poisson arrivals (exponential inter-arrival times) at rate lambda for class 0
auto arr = make_shared<exponential_distribution<double>>(lambda);
auto src = make_shared<des::source<double, exponential_distribution>>(
    vector<shared_ptr<exponential_distribution<double>>>{arr},
    "Source");

// M/M/1 station: 1 server, unlimited queue, exponential service at rate mu
auto svc = make_shared<exponential_distribution<double>>(mu);
auto sta = make_shared<des::station<double, exponential_distribution>>(
    vector<vector<shared_ptr<exponential_distribution<double>>>>{{{svc}}},
    1,        // 1 server
    1,        // 1 job per server
    1,        // 1 waiting queue
    INT_MAX,  // unlimited waiting queue
    "M/M/1");

auto snk = make_shared<des::sink>("Sink");
```

`des::station<double, exponential_distribution>` is the template instantiation for exponential service times. The distribution matrix is indexed `[server][class]`; with 1 server and 1 class it shrinks to `{{{svc}}}`. The event-class count is inferred from that distribution matrix.

---

### 3. Observers

```cpp
auto sojourn = make_shared<des::scalar>(NODE_SOJOURN, 1);
sta->attach(SIGNAL_NODE_DEPARTURE, sojourn);

des::ratio throughput("throughput", 1);
```

Every time a job departs the station `SIGNAL_NODE_DEPARTURE` is fired. The departure message carries the event's fields, including `NODE_SOJOURN` (total time in node = wait + service). The scalar was built with the `NODE_SOJOURN` tag, so `update(message)` reads exactly that field on each departure.

The [`des::ratio`](../api/observers.md#desratio) is not attached to a signal. The simulation loop (step 6) feeds it one pair per regeneration cycle: the jobs the station served in the cycle and the cycle's length. The ratio estimates the throughput, Σjobs / Σlength, with a confidence interval that accounts for the covariance of the two.

---

### 4. Network and Routing

```cpp
vector<vector<vector<double>>> routing = {
    {{0}, {1}, {0}},   // node 0 (source)  → node 1 with p=1
    {{0}, {0}, {1}},   // node 1 (station) → node 2 with p=1
    {{0}, {0}, {0}}    // node 2 (sink)    → nowhere
};

vector<shared_ptr<des::node>> nodes{src, sta, snk};
des::network net(nodes, routing, 42);
```

`routing[src][dst][cls]` is the routing probability. The values for a fixed source node and class must sum to 1. The last argument seeds the random streams: node `i` draws from its own streams, and so does the routing of the jobs leaving it. Events delivered to a node before the network is built would draw from the node's default streams, so the bootstrap (step 5) comes after this.

---

### 5. Bootstrap

```cpp
auto e = make_shared<des::event>();
e->set_cls(0);
e->set_time(0.0);
e->set_info(EVENT_NODE, 0);
nodes.at(0)->arrival(e);
nodes.clear();
```

The network does not auto-inject events. One bootstrap event must be placed at the source, which then schedules the first real arrival; the simulation is self-sustaining from that point.

---

### 6. Stopping Rules and the Simulation Loop

A **regeneration point** is a departure that leaves the station empty. Arrivals are Poisson, so the time to the next one is memoryless. At that point the model is back in its state at time 0, and what follows is independent of what came before. Two stopping rules build on it:

- **Within a run.** The cycles between regeneration points are independent, so the ratio's `run_confidence_interval()` is a valid interval for the run's throughput (the regenerative method). The run ends at the first regeneration point at which that interval is within `run_tolerance` (5%) of the estimate.
- **Across runs.** Every run starts and ends at a regeneration point, so the runs are independent and identically distributed, and none needs a warm-up period. Runs are added until the interval across runs, `confidence_interval()`, is within `tolerance` (1%) of its mean.

```cpp
const double alpha         = 0.05;                     // 95% confidence intervals
const double tolerance     = DES_TEST_TOLERANCE;       // across runs (0.01 by default)
const double run_tolerance = DES_TEST_RUN_TOLERANCE;   // within a run (0.05 by default)
const size_t min_runs      = DES_TEST_MIN_RUNS;        // runs before the first check across runs (10)
const long   min_cycles    = DES_TEST_MIN_CYCLES;      // cycles before the first check within a run (30)

// An interval is narrow enough when its half-width is within tol times its midpoint
bool narrow(pair<double, double> ci, double tol)
{
    return ci.second - ci.first <= tol * (ci.first + ci.second);
}

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
```

The origin of a departing event is read from `EVENT_NODE` before `route()`, because `route()` moves the event to its next node. When that node is the sink, the sink also returns the event to the pool. After `next_event()` the station has already released the job, so its occupancy, `queue_length() + service_length()`, tells whether the departure emptied it.

`net.reset(sim_time, {}, true)` cascades through every node and calls `obs->reset(true)` on each attached observer. The ratio is not attached to a node, so the loop resets it explicitly. `min_cycles` and `min_runs` keep the first, unreliable intervals, from a handful of samples, from stopping a run or the simulation too early. The `DES_TEST_*` values can be set at build time, e.g. `make test TEST_TOLERANCE=0.005` (see the Makefile).

---

### 7. Confidence Intervals

```cpp
replications r = replicate(net, *sta, throughput);

auto [thr_lo, thr_hi] = throughput.confidence_interval(alpha, 0);
auto [soj_lo, soj_hi] = sojourn->confidence_interval(alpha, 0);
```

Both are Student-t intervals across the runs. The throughput interval is built from the ratio's per-run estimates, Σjobs / Σlength, which equal the station's N/T for the run. The sojourn interval is built from the per-run means that the scalar stored at each `net.reset(..., true)`.

---

## Expected Output

```
M/M/1 open queueing system
  lambda = 0.8  mu = 1  rho = 0.8
  Theoretical mean sojourn = 5

Results over 29 replications (95% CI):
  42840 jobs served in 8779 regeneration cycles, simulated time 53916
  Throughput   [0.786936, 0.802772]  (theory: 0.8)
  Mean sojourn [4.47841, 5.14085]  (theory: 5)
```

This is the output with Apple clang. Other standard libraries draw different samples, so the number of replications and the intervals differ. The throughput interval stops only when it is within 1% of its mean. The sojourn interval is wider, because the sojourn times of a busy M/M/1 queue vary much more than its throughput.

---

## Key Concepts Demonstrated

| Concept | Code location |
|---|---|
| Seed of the random streams | `des::network net(nodes, routing, 42)` |
| Template station | `des::station<double, exponential_distribution>` |
| Observer attachment | `sta->attach(SIGNAL_NODE_DEPARTURE, sojourn)` |
| Bootstrap injection | `nodes.at(0)->arrival(e)` |
| Regeneration point | `sta.queue_length() + sta.service_length() == 0` after a departure |
| Regenerative CI within a run | `throughput.update(jobs, sim_time - start, 0)`, `throughput.run_confidence_interval(alpha, 0)` |
| Multi-run reset | `net.reset(sim_time, {}, true)`, `throughput.reset(true)` |
| Throughput CI across runs | `throughput.confidence_interval(alpha, 0)` |
| Sojourn CI | `sojourn->confidence_interval(alpha, 0)` |
