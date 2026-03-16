# des::station

**Header:** `libdes_station.hpp`
**Inherits:** `des::node`

`des::station<TT, T>` is a generic service station parameterised on a numeric type and a distribution template. It serves as an M/G/c/K queue where every component is configurable.

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
        unsigned int n_classes,
        unsigned int queue_capacity,
        std::string  description,
        std::shared_ptr<std::mt19937_64> gen);
```

| Parameter | Description |
|---|---|
| `dist` | 2-D matrix of distributions `[server_idx][class_idx]`. Each server may have a different distribution per class. |
| `n_servers` | Number of parallel servers |
| `server_capacity` | Maximum jobs per server (use `1` for standard servers, `INT_MAX` for IS) |
| `n_classes` | Number of event classes |
| `queue_capacity` | Maximum events in the waiting queue (`INT_MAX` = unlimited) |
| `description` | Human-readable label |
| `gen` | Shared RNG |

Several shorter overloads exist that omit optional parameters and apply sensible defaults (e.g. single class, unlimited queue).

---

## Distribution Access

```cpp
std::shared_ptr<T<TT>> get_rng(unsigned int& cls, unsigned int& idx);
```

Returns the distribution object for class `cls` on server `idx`.

---

## Internal Behaviour

`get_service()` samples the distribution assigned to the chosen server and event class, returning the service duration. The base class `des::node` then adds this duration to the current time to schedule the departure event.

`enqueue()` places a blocked event into the waiting queue.
`dequeue()` selects the next waiting event when a server becomes free.
`schedule()` assigns an event to a server.

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
                queues[qidx]->in_queue() != 0)
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
    1,        // 1 event class
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
