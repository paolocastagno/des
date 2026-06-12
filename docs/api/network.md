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

// With custom routing/fork handler
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        int (*fork_handler)(
            std::shared_ptr<event>,
            const std::vector<std::vector<std::vector<double>>>& routing,
            std::shared_ptr<std::mt19937_64>& gen),
        std::shared_ptr<std::mt19937_64>& gen);

// With custom block handler
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        std::pair<bool,int> (*block_handler)(
            std::shared_ptr<event>,
            int destination,
            const std::vector<std::vector<std::vector<double>>>& routing,
            std::shared_ptr<std::mt19937_64>& gen),
        std::shared_ptr<std::mt19937_64>& gen);

// With custom routing/fork and block handlers
network(std::vector<std::shared_ptr<node>> nodes,
        std::vector<std::vector<std::vector<double>>> routing,
        int (*fork_handler)(
            std::shared_ptr<event>,
            const std::vector<std::vector<std::vector<double>>>& routing,
            std::shared_ptr<std::mt19937_64>& gen),
        std::pair<bool,int> (*block_handler)(
            std::shared_ptr<event>,
            int destination,
            const std::vector<std::vector<std::vector<double>>>& routing,
            std::shared_ptr<std::mt19937_64>& gen),
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

**Routing/fork handler** — called before default routing to allow custom destination selection. Signature:

```cpp
int handler(std::shared_ptr<des::event> e,
            const std::vector<std::vector<std::vector<double>>>& routing,
            std::shared_ptr<std::mt19937_64>& gen);
```

Return the destination node index. Returning `-1`, or otherwise selecting no positive destination, leaves the event outside the network in the current implementation.

---

## Event Loop

```cpp
std::shared_ptr<event> next_event();
```

Returns the earliest scheduled departure and removes it from its node's server queue. If the internal heap is empty, it is rebuilt from the current nodes first; if no event is pending, `nullptr` is returned.

```cpp
void route(std::shared_ptr<event> e);
```

Reads the source node from `EVENT_NODE`, determines the destination node from the routing matrix or custom routing handler, calls `arrival()` on it, updates flow counters, and refreshes the heap entry for the affected destination. If the destination is full, the block handler is invoked.

---

## Flow Statistics

All flow statistics are maintained per `(source, destination, class)` triple.

```cpp
int    get_count(int source, int destination, int cls);
double flow(int source, int destination, int cls);
double get_flow(int source, int destination, int cls);
double get_flow_stddev(int source, int destination, int cls);
std::shared_ptr<observer> get_observer(const std::string& signal, unsigned int idx);
```

`get_count` returns the raw event count for the current run.
`get_flow` returns the estimated throughput (count / elapsed time), averaging across runs.
`get_flow_stddev` returns the standard deviation of throughput across runs.

### Confidence Intervals

```cpp
std::pair<double,double> get_count_ci(int src, int dst, int cls, double alpha);
std::pair<double,double> get_flow_ci(int src, int dst, int cls, double alpha);
std::vector<std::pair<double,double>> get_count_ci(int src, int dst, double alpha);
std::vector<std::pair<double,double>> get_flow_ci(int src, int dst, double alpha);
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
    net.route(e);
}

double throughput = net.get_flow(1, 2, 0);   // station -> sink throughput
double stddev     = net.get_flow_stddev(1, 2, 0);
```
