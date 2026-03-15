# des::source and des::sink

Both classes inherit from `des::sourcesink`, which in turn inherits from `des::node`.

---

## des::source

**Header:** `libdes_source.hpp`

A `source` generates events according to an exponential inter-arrival process.
It holds a single initial event that re-schedules itself after each departure, producing a Poisson arrival stream for each configured event class.

### Constructors

```cpp
source(std::vector<double> rates,
       std::string description,
       std::shared_ptr<std::mt19937_64> gen);

source(std::string description);
```

| Parameter | Description |
|---|---|
| `rates` | Vector of arrival rates, one per event class. Rate `λ` produces exponential inter-arrival times with mean `1/λ`. |
| `gen` | Shared RNG instance |

### Configuration

```cpp
void set_rate(double rate, int cls);
```

Changes the arrival rate for class `cls` after construction.

### Internal Behaviour

After each departure the source samples a new inter-arrival time from `exponential_distribution(rate[cls])` and schedules the next arrival event, maintaining a self-sustaining stream.

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
sink(std::string description, std::shared_ptr<std::mt19937_64> gen);
```

### Behaviour

`get_service()` always returns `0`, so every arriving event departs instantly. The sink fires the standard `SIGNAL_NODE_ARRIVAL` and `SIGNAL_NODE_DEPARTURE` signals, so observers can be attached to measure end-to-end sojourn times or count completed jobs.

---

## des::sourcesink (base)

**Header:** `libdes_sourcesink.hpp`

Shared base for `source` and `sink` that provides an **event pool** to reduce allocation pressure. Completed events are returned to the pool via `dispose_event()` and reused by `get_event()`.

```cpp
std::shared_ptr<event> get_event();
void                   dispose_event(std::shared_ptr<event> e);
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
