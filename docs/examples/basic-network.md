# Example: M/M/1 Open Queueing System

This example walks through `test/test.cpp`, which builds a minimal open queueing network, attaches a sojourn-time observer, and estimates throughput and mean sojourn time across multiple independent replications.

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

### 1. Random Number Generator

```cpp
auto gen = make_shared<mt19937_64>();
gen->seed(42);
```

A single seeded `mt19937_64` is shared with all components. Fixing the seed makes runs reproducible.

---

### 2. Nodes

```cpp
// Source: Poisson arrivals at rate lambda for class 0
auto src = make_shared<des::source>(vector<double>{lambda}, "Source", gen);

// M/M/1 station: 1 server, unlimited queue, exponential service at rate mu
auto svc = make_shared<exponential_distribution<double>>(mu);
auto sta = make_shared<des::station<double, exponential_distribution>>(
    vector<vector<shared_ptr<exponential_distribution<double>>>>{{{svc}}},
    1,        // 1 server
    1,        // 1 job per server
    1,        // 1 event class
    INT_MAX,  // unlimited waiting queue
    "M/M/1", gen);

auto snk = make_shared<des::sink>("Sink");
```

`des::station<double, exponential_distribution>` is the template instantiation for exponential service times. The distribution matrix is indexed `[server][class]`; with 1 server and 1 class it shrinks to `{{{svc}}}`.

---

### 3. Observer

```cpp
auto sojourn = make_shared<des::scalar>(NODE_SOJOURN, 1);
sta->attach(SIGNAL_NODE_DEPARTURE, sojourn);
```

Every time a job departs the station `SIGNAL_NODE_DEPARTURE` is fired. The node serialises the departure message with keys including `NODE_SOJOURN` (total time in node = wait + service). The scalar's description is also `NODE_SOJOURN`, so `update(message)` extracts exactly that key on each departure.

---

### 4. Network and Routing

```cpp
vector<vector<vector<double>>> routing = {
    {{0}, {1}, {0}},   // node 0 (source)  → node 1 with p=1
    {{0}, {0}, {1}},   // node 1 (station) → node 2 with p=1
    {{0}, {0}, {0}}    // node 2 (sink)    → nowhere
};

vector<shared_ptr<des::node>> nodes{src, sta, snk};
des::network net(nodes, routing, gen);
```

`routing[src][dst][cls]` is the routing probability. The values for a fixed source node and class must sum to 1.

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

### 6. Simulation Loop

```cpp
const int N_EVENTS = 100000;
const int N_RUNS   = 5;
int run = 0;

do {
    double sim_time = 0.0;
    for (int i = 0; i < N_EVENTS; ++i)
    {
        e = net.next_event();          // find globally earliest event
        sim_time = e->get_time();
        net.route(e, sim_time);        // route it → fires observer notifications
    }

    double mean_soj = sojourn->run_avg(0);   // v/n = sample mean for this run
    sojourn->stddev(0);   // updates internal cross-run mean used by CI

    cout << "Run " << run
         << "  sim_time = " << sim_time
         << "  mean sojourn = " << mean_soj << "\n";

    // Resets nodes, queues, and all attached observers.
    // newrun=true stores the current run's result for cross-run CI.
    net.reset(sim_time, {}, true);
}
while (++run < N_RUNS);
```

`net.reset(sim_time, {}, true)` cascades through every node and calls `obs->reset(true)` on each attached observer. This saves the run's statistics before clearing within-run state.

The call to `sojourn->stddev(0)` immediately before the reset propagates the per-run mean into the scalar's internal cross-run accumulator (`s`). Without it, `confidence_interval` would see zero-valued samples.

---

### 7. Confidence Intervals

```cpp
auto [thr_lo, thr_hi] = net.get_flow_ci(1, 2, 0, 0.05);
auto [soj_lo, soj_hi] = sojourn->confidence_interval(0.05, 0);

cout << "Throughput   [" << thr_lo << ", " << thr_hi << "]  (theory: " << lambda << ")\n"
     << "Mean sojourn [" << soj_lo << ", " << soj_hi << "]  (theory: " << 1.0/(mu-lambda) << ")\n";
```

`get_flow_ci(src, dst, cls, alpha)` returns a Student-t CI on the between-run throughput estimates. The sojourn CI is computed the same way from the 5 per-run means stored internally.

---

## Expected Output

```
M/M/1 open queueing system
  lambda = 0.8  mu = 1  rho = 0.8
  Theoretical mean sojourn = 5

Run 0  sim_time = ...  mean sojourn ≈ 5.x
Run 1  sim_time = ...  mean sojourn ≈ 5.x
...

Results over 5 replications (95% CI):
  Throughput   [0.79x, 0.80x]  (theory: 0.8)
  Mean sojourn [4.8x,  5.2x]   (theory: 5)
```

---

## Key Concepts Demonstrated

| Concept | Code location |
|---|---|
| Shared RNG seed | `gen->seed(42)` |
| Template station | `des::station<double, exponential_distribution>` |
| Observer attachment | `sta->attach(SIGNAL_NODE_DEPARTURE, sojourn)` |
| Bootstrap injection | `nodes.at(0)->arrival(e)` |
| Multi-run reset | `net.reset(sim_time, {}, true)` |
| Throughput CI | `net.get_flow_ci(1, 2, 0, 0.05)` |
| Sojourn CI | `sojourn->confidence_interval(0.05, 0)` |
