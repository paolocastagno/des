# des::station

**Header:** `libdes_station.hpp`
**Inherits:** `des::node`

`des::station<TT, T>` is a generic service station parameterised on a numeric type and a distribution template. It serves as an M/G/c/K queue where queues, server capacity, and queue policies are configurable.

---

## Template Parameters

| Parameter | Description |
|---|---|
| `TT` | Numeric type for service-time samples (usually `double`) |
| `T` | Distribution template (e.g. `exponential_distribution`, `piecewise_constant_distribution`) |

Any distribution from `<random>` that has an `operator()(RNG&)` returning `TT` is compatible.

---

## Constructors

```cpp
station(std::vector<std::vector<std::shared_ptr<T<TT>>>> dist,
        unsigned int n_servers,
        unsigned int server_capacity,
        unsigned int n_queues,
        unsigned int queue_capacity,
        std::string  description,
        std::shared_ptr<std::mt19937_64> gen);

station(std::vector<std::vector<std::shared_ptr<T<TT>>>> dist,
        unsigned int n_servers,
        unsigned int server_capacity,
        unsigned int n_queues,
        unsigned int queue_capacity,
        std::shared_ptr<des::policy> queue_policy,
        std::shared_ptr<des::policy> server_policy,
        std::string  description,
        std::shared_ptr<std::mt19937_64> gen);

station(std::vector<std::vector<std::shared_ptr<T<TT>>>> dist,
        unsigned int n_servers,
        unsigned int server_capacity,
        std::shared_ptr<des::policy> server_policy,
        std::string description,
        std::shared_ptr<std::mt19937_64> gen);
```

| Parameter | Description |
|---|---|
| `dist` | 2-D matrix of distributions `[server_idx][class_idx]`. Each server may have a different distribution per class; the class count is inferred from `dist[0].size()`. |
| `n_servers` | Number of parallel servers |
| `server_capacity` | Maximum jobs per server (use `1` for standard servers, `INT_MAX` for IS) |
| `n_queues` | Number of waiting queues |
| `queue_capacity` | Maximum events in the waiting queue (`INT_MAX` = unlimited) |
| `queue_policy` | Queue policy for waiting queues (`des::fifo` by default) |
| `server_policy` | Queue policy for server queues (`des::fifo`, `des::is`, `des::ps`, etc.) |
| `description` | Human-readable label |
| `gen` | Shared RNG |

Additional overloads accept a custom service-pick handler, omit the waiting queues for server-only stations, or use `station(dist, description, gen)` as a one-server infinite-server-style convenience constructor.

---

## Distribution Access

```cpp
std::shared_ptr<T<TT>> get_rng(unsigned int& cls, unsigned int& idx);
```

Returns the distribution object for class `cls` on server `idx`.

---

## Internal Behaviour

`get_service()` samples the distribution assigned to the chosen server and event class, returning the service duration. The base class `des::node` then adds this duration to the current time to schedule the departure event.

`enqueue()` returns the waiting-queue index for a blocked event.
`dequeue()` returns the waiting-queue index for the next event to move into service.
`schedule()` returns the server index for an arriving or newly scheduled event.

The station can also use a **custom service-pick handler** to choose from which waiting queue the next job should be moved to service (for example strict class priority).

Handler signature:

```cpp
int service_pick_handler(std::shared_ptr<des::event> trigger,
                         int server_idx,
                         const std::vector<std::vector<int>>& q_map,
                         const std::vector<std::shared_ptr<des::queue>>& queues,
                         std::shared_ptr<std::mt19937_64>& gen);
```

If no handler is provided, `des::station` keeps the default behavior.

## Example: Strict class priority for service admission

```cpp
int class_priority_pick(std::shared_ptr<des::event> trigger,
                        int server_idx,
                        const std::vector<std::vector<int>>& q_map,
                        const std::vector<std::shared_ptr<des::queue>>& queues,
                        std::shared_ptr<std::mt19937_64>& gen)
{
    (void)trigger;
    (void)server_idx;
    (void)gen;

    // Priority order: class 2 > class 0 > class 1
    std::vector<int> prio{2, 0, 1};

    int fallback = 0;
    for (unsigned int qidx = 0; qidx < q_map.size(); ++qidx)
    {
        if (!q_map[qidx].empty()) fallback = static_cast<int>(qidx);
    }

    for (int cls : prio)
    {
        for (unsigned int qidx = 0; qidx < q_map.size(); ++qidx)
        {
            if (cls >= 0 && cls < static_cast<int>(q_map[qidx].size()) &&
                q_map[qidx][cls] != 0 &&
                queues[qidx]->size() != 0)
            {
                return static_cast<int>(qidx);
            }
        }
    }

    return fallback;
}
```

Pass this function pointer to a station constructor overload that accepts the custom handler.

---

## Example: Processor Sharing (PS) Station

A PS station has a single logical server with unlimited capacity and a `des::ps` policy. Every arriving job enters service immediately and shares the CPU equally with all other jobs present.

```cpp
#include "libdes_ps.hpp"

auto mu = std::make_shared<exponential_distribution<double>>(1.0);

auto ps_sta = std::make_shared<des::station<double, exponential_distribution>>(
    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{{{mu}}},
    1,                                          // 1 server
    std::numeric_limits<unsigned int>::max(),   // unlimited → never full
    std::make_shared<des::ps>(),
    "PS",
    gen);
```

For **GPS** with two classes where class 0 receives twice the CPU share of class 1:

```cpp
// dist[server][class]
auto mu0 = std::make_shared<exponential_distribution<double>>(2.0);
auto mu1 = std::make_shared<exponential_distribution<double>>(1.0);

auto gps_sta = std::make_shared<des::station<double, exponential_distribution>>(
    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{
        {mu0, mu1}  // server 0: class 0 and class 1 distributions
    },
    1,
    std::numeric_limits<unsigned int>::max(),
    std::make_shared<des::ps>(std::vector<double>{2.0, 1.0}),  // weights per class
    "GPS",
    gen);
```

---

## Example: M/M/2 Queue

```cpp
auto mu = std::make_shared<exponential_distribution<double>>(1.0);

// Two servers, each using the same exponential distribution for class 0
auto sta = std::make_shared<des::station<double, exponential_distribution>>(
    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{
        {mu},  // server 0, class 0
        {mu}   // server 1, class 0
    },
    2,        // 2 servers
    1,        // 1 job per server
    1,        // 1 waiting queue
    100,      // queue capacity
    "M/M/2",
    gen);
```

## Example: Infinite Server (IS) with Piecewise-Constant Distribution

```cpp
std::vector<double> intervals{0.0, 1.0};
std::vector<double> weights{1.0};
auto dist = std::make_shared<piecewise_constant_distribution<double>>(
                intervals.begin(), intervals.end(), weights.begin());

auto is = std::make_shared<des::station<double, piecewise_constant_distribution>>(
    std::vector<std::vector<std::shared_ptr<piecewise_constant_distribution<double>>>>{
        {dist}
    },
    1, INT_MAX,   // 1 server entry, unlimited capacity -> IS
    "IS",
    gen);
```
