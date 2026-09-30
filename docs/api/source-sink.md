# des::source and des::sink

Both classes inherit from `des::sourcesink`, which in turn inherits from `des::node`.

---

## des::source

**Header:** `libdes_source.hpp`

`des::source<TT, T>` generates events whose inter-arrival times follow a distribution of your choice.
Each bootstrap event re-schedules itself after each departure, producing a renewal arrival stream for its class (a Poisson stream with `exponential_distribution`).

### Template Parameters

| Parameter | Description |
|---|---|
| `TT` | Numeric type for inter-arrival samples (usually `double`) |
| `T` | Distribution template (e.g. `exponential_distribution`, `gamma_distribution`, `uniform_real_distribution`) |

The library instantiates `des::source` for the same `<random>` distributions as [`des::station`](station.md).

### Constructors

```cpp
source(std::vector<std::shared_ptr<T<TT>>> dist,
       std::string description);

source(std::string description);
```

| Parameter | Description |
|---|---|
| `dist` | Inter-arrival time distributions, one per event class; the class count is `dist.size()`. |

Each class draws its inter-arrival times from its own stream (see [random streams](node.md#random-streams)), so changing the distribution of one class leaves the arrival times of the others unchanged.

`source(description)` builds a one-class source with no distribution, for tests and custom subclasses; it cannot generate arrivals.

### Distribution Access

```cpp
std::shared_ptr<T<TT>> get_rng(unsigned int cls) const;
void                   set_rng(std::shared_ptr<T<TT>> dist, unsigned int cls);
```

`get_rng` returns the distribution of class `cls`. It is shared with the source, so changing its parameters changes the arrivals the source generates. `set_rng` replaces it. Either change applies from the next inter-arrival time drawn: the arrival already scheduled keeps its time.

### Example: Poisson and Erlang arrivals

```cpp
// Poisson arrivals at rate 0.8
auto exp_arr = std::make_shared<std::exponential_distribution<double>>(0.8);
auto poisson = std::make_shared<des::source<double, std::exponential_distribution>>(
    std::vector<std::shared_ptr<std::exponential_distribution<double>>>{exp_arr},
    "Poisson");

// Erlang-4 inter-arrival times with mean 1.25 (rate 0.8, squared coefficient of variation 1/4)
auto erl_arr = std::make_shared<std::gamma_distribution<double>>(4.0, 1.25 / 4.0);
auto erlang = std::make_shared<des::source<double, std::gamma_distribution>>(
    std::vector<std::shared_ptr<std::gamma_distribution<double>>>{erl_arr},
    "Erlang");
```

### Internal Behaviour

After each departure the source samples a new inter-arrival time from the distribution of the event's class, clones the departed event into a recycled event object, and schedules the next arrival event. This keeps the external arrival stream self-sustaining after the bootstrap event.

### Serialisation

```cpp
std::string to_string();
```

---

## des::sink

**Header:** `libdes_sink.hpp`

A `sink` absorbs all events that arrive at it; no service time is generated and events are simply discarded. It is always the terminal node of a simulation network.

### Constructors

```cpp
sink(std::string description);
sink(std::string description, int cls);
```

Use `sink(description, cls)` for multi-class simulations. A legacy rates-based overload is declared in the header for compatibility, but the sink does not use service rates or random sampling.

### Behaviour

The sink absorbs every arriving event inside `arrival()`: it records the arrival and the departure of the job at the same time, then returns the event to the pool. It never schedules a departure event, so `network::next_event()` returns no event from a sink, and a run of N events covers only station and source events. The sink still fires the standard `SIGNAL_NODE_ARRIVAL`, `SIGNAL_NODE_SERVICE` and `SIGNAL_NODE_DEPARTURE` signals (with a zero sojourn), so observers can be attached to count completed jobs.

---

## des::sourcesink (base)

**Header:** `libdes_sourcesink.hpp`

Shared base for `source` and `sink` that provides an **event pool** to reduce allocation pressure. Completed events are returned to the pool via `dispose_event()` and reused by `get_event()`.

```cpp
std::shared_ptr<event> get_event();
void                   dispose_event(const std::shared_ptr<event>& e);
```

---

## Injecting the First Event

The network does not inject the bootstrap event automatically. You must create one, configure it, and call `arrival()` on the source before starting the loop:

```cpp
auto e = std::make_shared<des::event>();
e->set_cls(0);
e->set_time(0.0);
e->set_info(EVENT_NODE, 0);   // node index 0 = source
src->arrival(e);
```

After this call the source schedules the first real arrival and the main loop can start.
