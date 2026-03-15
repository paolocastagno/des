# des::network

**Header:** `libdes_network.hpp`
**Inherits:** `des::observable`

`des::network` ties together a set of nodes and a routing matrix. It provides the global event-loop helpers (`next_event()` / `route()`) and maintains flow statistics between node pairs.

---

## Constructors

```cpp
// Basic constructor
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        std::shared_ptr<std::mt19937_64>& gen);

// With custom block handler
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        std::function<std::pair<bool,int>(std::shared_ptr<event>, int,
            const std::vector<std::vector<std::vector<double>>>&,
            std::shared_ptr<std::mt19937_64>&)> block_handler,
        std::shared_ptr<std::mt19937_64>& gen);

// With custom fork and block handlers
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        std::function<...> fork_handler,
        std::function<...> block_handler,
        std::shared_ptr<std::mt19937_64>& gen);
```

### Routing Matrix

`routing[src][dst][cls]` is the probability that an event of class `cls` departing node `src` is sent to node `dst`. The values for a fixed `src` and `cls` must sum to 1.

Nodes are indexed in the order they appear in the `nodes` vector.

### Custom Handlers

**Block handler** — called when the chosen destination is full. Signature:

```cpp
std::pair<bool, int> handler(std::shared_ptr<des::event> e,
                             int destination,
                             const std::vector<std::vector<std::vector<double>>>& routing,
                             std::shared_ptr<std::mt19937_64>& gen);
```

Return `{true, alternative_destination}` to reroute, or `{false, _}` to drop the event.

**Fork handler** — called before routing to allow splitting or custom destination selection.

---

## Event Loop

```cpp
std::shared_ptr<event> next_event();
```

Scans all nodes for the earliest scheduled departure and returns that event (removing it from its node's server queue).

```cpp
void route(std::shared_ptr<event> e, const double& time);
```

Determines the destination node from the routing matrix (sampling `routing[src][dst][cls]`), calls `arrival()` on it, and updates flow counters. If the destination is full, the block handler is invoked.

---

## Flow Statistics

All flow statistics are maintained per `(source, destination, class)` triple.

```cpp
double get_count(int source, int destination, int cls);
double flow(int source, int destination, int cls);
double get_flow(int source, int destination, int cls);
double get_flow_stddev(int source, int destination, int cls);
```

`get_count` returns the raw event count for the current run.
`get_flow` returns the estimated throughput (count / elapsed time), averaging across runs.
`get_flow_stddev` returns the standard deviation of throughput across runs.

### Confidence Intervals

```cpp
std::pair<double,double> get_count_ci(int src, int dst, int cls, double alpha = 0.05);
std::pair<double,double> get_flow_ci(int src, int dst, int cls, double alpha = 0.05);
```

---

## Reset

```cpp
void reset(double sim_time, std::vector<std::string> keys, bool newrun);
```

Resets all nodes and flow counters. Passing `newrun = true` stores the current-run statistics for between-run aggregation (used for confidence interval estimation across multiple replications).

---

## Serialisation

```cpp
std::string to_string();
```

---

## Example

```cpp
std::vector<std::vector<std::vector<double>>> routing = {
    {{0}, {1}, {0}},   // node 0 (source)  -> 100% to node 1
    {{0}, {0}, {1}},   // node 1 (station) -> 100% to node 2
    {{0}, {0}, {0}}    // node 2 (sink)    -> nowhere
};

des::network net({src, sta, snk}, routing, gen);

for (int i = 0; i < 100000; ++i)
{
    auto e = net.next_event();
    net.route(e, e->get_time());
}

double throughput = net.get_flow(1, 2, 0);   // station -> sink throughput
double stddev     = net.get_flow_stddev(1, 2, 0);
```
