# Architecture

## Overview

libdes models a simulation as a **network of autonomous nodes** connected by a probabilistic routing table. The simulation engine advances time by repeatedly asking the network for the next scheduled event, routing it to the appropriate node, and collecting measurements via attached observers.

![libdes architecture: your program seeds and drives the event loop of a des::network, here a central server model in which jobs alternate between a processor-sharing CPU and a FIFO disk before leaving through a sink; every node draws from its own block of random streams; every queue delegates to a policy and the job store it creates; observers attached to node signals collect the statistics](../figures/architecture.svg)

The numbered steps are one iteration of the [simulation loop](#simulation-loop). The network is a central server model: after each burst at the processor-sharing CPU, a job of class c visits the FIFO disk with probability p[c], otherwise it leaves. The two stations hold their jobs in different [job stores](#3-policy-pattern-queue-disciplines): a `ps_store` and a `sequence_store`. The dice mark each node's block of [random streams](#5-random-streams). Dashed parts are extension points: queue policies and job stores, inter-arrival and service-time distributions, routing and block handlers, and observers.

---

## Class Hierarchy

```
des::object
├── des::event
└── des::queue
    └── (owns a des::policy)

des::observable
└── des::node  (also inherits des::object)
    ├── des::sourcesink
    │   ├── des::source<TT,T>
    │   └── des::sink
    └── des::station<TT,T>

des::network  (also inherits des::observable)

des::observer
├── des::scalar
├── des::counter
├── des::sample
├── des::histogram
└── des::ratio

des::policy
├── des::fifo
├── des::is
└── des::ps

des::message
```

---

## Key Design Patterns

### 1. Future Event List (FEL) per Node

Each node owns one or more `des::queue` objects that act as its local Future Event List. The `des::network` polls all nodes and always processes the globally earliest event first, preserving causal order.

### 2. Observer Pattern

Nodes inherit from `des::observable`. Any `des::observer` subclass (`scalar`, `counter`, `sample`, `histogram`) can be **attached** to a node on a named signal (e.g. `SIGNAL_NODE_ARRIVAL`, `SIGNAL_NODE_DEPARTURE`). When the node fires that signal it passes a `des::message` view of the event's fields to all registered observers, which read the fields they measure by [tag](api/tags.md). `des::ratio` is the exception: it is fed directly with pairs, such as the jobs served in a regeneration cycle and the cycle's length, and estimates Σx / Σy with a confidence interval (see [Multi-Run Support](#multi-run-support)).

```cpp
auto sojourn = std::make_shared<des::scalar>(NODE_SOJOURN, 1);   // measures the NODE_SOJOURN field
myNode->attach(SIGNAL_NODE_DEPARTURE, sojourn);
```

### 3. Policy Pattern (Queue Disciplines)

Queue behaviour is encapsulated in a `des::policy` subclass. The built-in policies are:

| Class | Discipline | Store |
|---|---|---|
| `des::fifo` | First-In First-Out | `des::sequence_store` (ring buffer in time order) |
| `des::is` | Infinite Server (no waiting) | `des::time_store` (binary heap) |
| `des::ps` | Processor Sharing / Generalized Processor Sharing (GPS) | `des::ps_store` (heap on virtual finish tags) |

A policy has two responsibilities: it decides which jobs a queue admits (`admit()`), and it creates the `des::job_store` that holds the queue's jobs (`make_store()`). The store owns the container and the release order, including time-dependent bookkeeping such as the processor-sharing virtual time. `des::queue` and `des::node` only use the abstract `job_store` interface, so a new discipline is added by deriving a store and a policy, without changing the library (see [Implementing a custom policy](api/queue.md#implementing-a-custom-policy)). A policy may be shared by several queues: each queue asks it for its own store.

### 4. Template-Based Distributions (Station)

`des::station<TT, T>` is parameterised on a numeric type `TT` and a distribution template `T`. Any distribution from `<random>` (or a compatible custom class) can be plugged in:

```cpp
// Exponential service
des::station<double, exponential_distribution> mm1(...);

// Piecewise-constant service
des::station<double, piecewise_constant_distribution> is_node(...);
```

### 5. Random streams

Every source of randomness draws from its own stream of a single xoshiro256** generator (`des::random_engine`, `libdes_random.hpp`), seeded once through the network constructor. The streams are disjoint pieces of the generator's period, obtained by jump-ahead, the approach of L'Ecuyer's RngStreams:

| Stream | Used for |
|---|---|
| block 2i, stream 0 | choices of node i among its queues and servers |
| block 2i, stream 1 + c | service (or inter-arrival) times of class c at node i |
| block 2i+1, stream c | routing of the class-c events leaving node i |

Blocks are 2^192 draws apart (`long_jump()`) and the streams inside a block 2^128 draws apart (`jump()`), so no stream can run into another. The seed is expanded into the generator state with SplitMix64, so similar seeds give unrelated starting points, and streams of different seeds overlap with negligible probability.

A stream depends only on the node's index in the network, not on the order in which the components are built. Adding a node after the others, or changing the rates of one class, leaves every other stream unchanged. Comparisons between model variants therefore use common random numbers, which reduces the variance of the estimated differences. Nodes used outside a network draw from a default block, and `node::set_streams()` assigns a block explicitly.

### 6. Event Routing via Routing Matrix

The routing between nodes is expressed as a 3-D matrix `routing[src][dst][cls]` where the value is the probability that an event of class `cls` departing node `src` is forwarded to node `dst`. The `des::network` samples this distribution on each departure.

Custom **fork** and **block** handlers (function pointers) can override the default routing logic for special cases such as conditional branching or backpressure.

---

## Simulation Loop

A typical main loop looks like this:

```cpp
for (int i = 0; i < N_EVENTS; ++i)
{
    auto e = net.next_event();    // 1. Find globally earliest event
    net.route(e);                 // 2. Process it (generate next events, notify observers)
}
```

`next_event()` scans all nodes for their minimum scheduled time, removes that event from its queue, and returns it.

`route()` reads the event's current node from `EVENT_NODE`, determines the destination node from the routing matrix, calls `arrival()` on it, updates network flow observers, and refreshes the next-event heap for the affected node.

---

## Multi-Run Support

Each node and observer supports `reset(double time, vector<des::tag> keys, bool newrun)`, where `keys` lists the event fields holding times to shift along with the clock. Calling `net.reset(sim_time, {}, true)` at the end of a run preserves between-run statistics (mean of means, between-run variance) while clearing within-run state, enabling confidence-interval estimation across multiple independent replications.

Runs are independent when each starts and ends at a **regeneration point**: a state from which the model's future does not depend on its past. In an M/M/1 queue, for example, that is a departure that leaves the station empty, because the time to the next Poisson arrival is memoryless. Such runs need no warm-up period. The cycles between regeneration points are also independent within a run. A `des::ratio` fed with one pair per cycle (the jobs served and the cycle's length) gives the run's throughput with a confidence interval, so a run can end at the first regeneration point at which that interval is narrow enough. [A first model](../README.md#a-first-model) and `test/test.cpp` stop both runs and replications this way.

---

## Thread Safety

The library is **not thread-safe**. All simulation state is mutated synchronously inside the main event loop. Use separate process instances for parallel replications if needed.
