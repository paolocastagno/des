# Architecture

## Overview

libdes models a simulation as a **network of autonomous nodes** connected by a probabilistic routing table. The simulation engine advances time by repeatedly asking the network for the next scheduled event, routing it to the appropriate node, and collecting measurements via attached observers.

```
┌──────────────────────────────────────────────────────────┐
│                        des::network                       │
│                                                          │
│  ┌──────────┐   routing   ┌──────────┐   routing         │
│  │  source  │ ──────────► │ station  │ ──────────► sink  │
│  └──────────┘             └──────────┘                   │
│       │                        │                         │
│  observers                observers                      │
└──────────────────────────────────────────────────────────┘
```

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
    │   ├── des::source
    │   └── des::sink
    └── des::station<TT,T>

des::network  (also inherits des::observable)

des::observer
├── des::scalar
├── des::counter
├── des::vector
└── des::histogram

des::policy
├── des::fifo
└── des::is

des::message
```

---

## Key Design Patterns

### 1. Future Event List (FEL) per Node

Each node owns one or more `des::queue` objects that act as its local Future Event List. The `des::network` polls all nodes and always processes the globally earliest event first, preserving causal order.

### 2. Observer Pattern

Nodes inherit from `des::observable`. Any `des::observer` subclass (`scalar`, `counter`, `vector`, `histogram`) can be **attached** to a node on a named signal (e.g. `SIGNAL_NODE_ARRIVAL`, `SIGNAL_NODE_DEPARTURE`). When the node fires that signal it serialises relevant state into a `des::message` and calls `notify()`, which forwards the message to all registered observers.

```cpp
auto throughput = std::make_shared<des::scalar>("throughput", 1);
myNode->attach("throughput", SIGNAL_NODE_DEPARTURE, throughput);
```

### 3. Policy Pattern (Queue Disciplines)

Queue behaviour is encapsulated in a `des::policy` subclass. The built-in policies are:

| Class | Discipline |
|---|---|
| `des::fifo` | First-In First-Out |
| `des::is` | Infinite Server (no waiting) |

Custom disciplines are implemented by subclassing `des::policy` and overriding the three `update()` overloads and `front()`.

### 4. Template-Based Distributions (Station)

`des::station<TT, T>` is parameterised on a numeric type `TT` and a distribution template `T`. Any distribution from `<random>` (or a compatible custom class) can be plugged in:

```cpp
// Exponential service
des::station<double, exponential_distribution> mm1(...);

// Piecewise-constant service
des::station<double, piecewise_constant_distribution> is_node(...);
```

### 5. Shared RNG

A single `std::mt19937_64` instance is passed by `shared_ptr` to every component that needs randomness. This makes runs reproducible by fixing the seed once at program start, and avoids the overhead of multiple independent generators.

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
    net.route(e, e->get_time());  // 2. Process it (generate next events, notify observers)
}
```

`next_event()` scans all nodes for their minimum scheduled time, removes that event from its queue, and returns it.

`route()` determines the destination node from the routing matrix, calls `arrival()` on it, which in turn calls `get_service()` and schedules the departure event.

---

## Multi-Run Support

Each node and observer supports `reset(double time, vector<string> keys, bool newrun)`. Calling `net.reset(sim_time, {}, true)` at the end of a run preserves between-run statistics (mean of means, between-run variance) while clearing within-run state, enabling confidence-interval estimation across multiple independent replications.

---

## Thread Safety

The library is **not thread-safe**. All simulation state is mutated synchronously inside the main event loop. Use separate process instances for parallel replications if needed.
