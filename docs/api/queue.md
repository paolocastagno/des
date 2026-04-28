# Queue and Queue Policies

## des::queue

**Header:** `libdes_queue.hpp`
**Inherits:** `des::object`

A `queue` manages an ordered collection of events according to a pluggable `policy`. Nodes own one or more queues (waiting queue and server queue).

### Constructors

```cpp
queue();
queue(std::shared_ptr<policy> pol);
queue(int positions, std::shared_ptr<policy> pol);
queue(unsigned int positions, std::shared_ptr<policy> pol);
```

| Parameter | Description |
|---|---|
| `positions` | Maximum capacity (`INT_MAX` = unlimited) |
| `pol` | Queue discipline (e.g. `des::fifo`, `des::is`) |

### Key Methods

```cpp
bool     is_full();
unsigned in_queue();          // current occupancy
double   min_time();          // earliest event time in the queue
```

#### Dequeue overloads

```cpp
shared_ptr<event> dequeue();             // basic pop (no time context)
shared_ptr<event> dequeue(double time);  // pop with time context
```

`dequeue(double time)` calls `policy::on_dequeue(list, time)` **before** removing the element. This allows time-aware policies (such as Processor Sharing) to update the departure times of the remaining jobs. For policies that do not override `on_dequeue` the two overloads are equivalent.

### Configuration

```cpp
int                    get_positions();
void                   set_positions(int positions);
std::shared_ptr<policy> get_policy();
void                   set_policy(policy* pol);
```

### Reset / Clear

```cpp
void reset(double time, std::vector<std::string> keys, bool newrun);
void clear();
```

---

## des::policy (abstract)

**Header:** `libdes_policy.hpp`

Abstract base class that defines the interface for queue disciplines.

### Interface

```cpp
// Called by queue::enqueue() to decide whether to admit the event and
// how to order it in the list.  Returns true to admit, false to reject.
virtual bool update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    unsigned int positions,
                    double time) = 0;

virtual bool update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    unsigned int positions) = 0;

virtual bool update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    double time) = 0;

// Returns true to dequeue from the front, false to dequeue from the back.
virtual bool front() = 0;

// Called by queue::dequeue(time) before the element is popped.
// Override to update remaining events when a departure changes the queue state.
// Default implementation is a no-op; existing policies are unaffected.
virtual void on_dequeue(std::list<std::shared_ptr<event>>& l, double time) {}
```

`update()` decides whether to admit `e` into the list `l` and may reorder or modify elements before insertion. `front()` controls which end of the sorted list is popped on dequeue. `on_dequeue()` is a hook for policies that need to react to departures (e.g. Processor Sharing).

### Description

```cpp
std::string get_description();
void        set_description(std::string d);
```

---

## des::fifo

**Header:** `libdes_fifo.hpp`
**Inherits:** `des::policy`

Implements a **First-In First-Out** discipline. Events are appended to the back of the list and removed from the front.

```cpp
des::fifo();
```

---

## des::is

**Header:** `libdes_is.hpp`
**Inherits:** `des::policy`

Implements the **Infinite Server** discipline. All arriving events are inserted in time order (earliest first) and there is no waiting — every event goes directly to a server. Used together with `INT_MAX` server capacity.

```cpp
des::is();
```

---

## des::ps

**Header:** `libdes_ps.hpp`
**Inherits:** `des::policy`

Implements **Processor Sharing (PS)** and its weighted generalisation **Generalized Processor Sharing (GPS)**. All admitted jobs share the CPU simultaneously; each class `c` carries a positive weight `w_c` and receives a CPU share proportional to that weight.

With total weight `W = Σ w_{c_k}` over all jobs currently in service, a job of class `c` is served at instantaneous rate `w_c / W`. Setting all weights to `1` (the default) gives standard equal-share PS.

### Constructors

```cpp
des::ps();                                // standard PS — all classes share equally
des::ps(std::vector<double> weights);     // GPS — weights[c] is the weight for class c
```

Classes with index beyond `weights.size()` default to weight `1.0`.

### Invariant

For every job `j` of class `c_j` in the server at time `t`:

```
event_time(j) = t + r_j * W / w_{c_j}
```

where `r_j` is the *equivalent remaining service time* — the work job `j` would still need if served alone at unit rate.

**On arrival** of a new job (weight `w_new`, raw service time `s_new`):

```
W' = W + w_new
existing jobs:  d_j'  = t + (d_j  - t) * W' / W       (class-independent scale)
new job:        d_new = t + s_new * W' / w_new
```

**On departure** of the front job (weight `w_dep`):

```
W' = W - w_dep
remaining jobs: d_j' = t + (d_j - t) * W' / W         (class-independent scale)
```

In both cases the scale factor `W'/W` is the same for all jobs regardless of class, so their relative sorted order is preserved and no list reordering is needed.

### Usage

Attach `des::ps` to the **server** queue of a station with unlimited capacity. Because the server is never full, every arriving job goes directly into service — no separate waiting queue is needed.

```cpp
// Standard PS station (single class, exponential service, unlimited server)
auto mu  = std::make_shared<exponential_distribution<double>>(1.0);
auto ps_policy = std::make_shared<des::ps>();

auto sta = std::make_shared<des::station<double, exponential_distribution>>(
    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{{mu}},
    1,                                         // 1 server
    std::numeric_limits<unsigned int>::max(),  // unlimited capacity → never full
    ps_policy,
    "PS station",
    gen);

// GPS station: class 0 gets twice the share of class 1
auto gps_policy = std::make_shared<des::ps>(std::vector<double>{2.0, 1.0});
```

---

## Implementing a Custom Policy

Subclass `des::policy` and implement the three `update()` overloads and `front()`. Override `on_dequeue()` only if your discipline needs to react when a job leaves (e.g. to reschedule remaining jobs):

```cpp
class my_lifo : public des::policy {
public:
    bool update(std::shared_ptr<des::event> e,
                std::list<std::shared_ptr<des::event>>& l,
                unsigned int positions, double time) override
    {
        if(l.size() >= positions) return false;
        l.push_front(e);   // LIFO: insert at front
        return true;
    }
    bool update(std::shared_ptr<des::event> e,
                std::list<std::shared_ptr<des::event>>& l,
                unsigned int positions) override { /* ... */ }
    bool update(std::shared_ptr<des::event> e,
                std::list<std::shared_ptr<des::event>>& l,
                double time) override { /* ... */ }

    bool front() override { return false; } // dequeue from back → LIFO

    // Optional: called before each pop.  Default no-op is sufficient for
    // disciplines whose remaining jobs are unaffected by a departure.
    // void on_dequeue(std::list<std::shared_ptr<des::event>>& l, double time) override { ... }
};
```
