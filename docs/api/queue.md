# Queues, Policies and Job Stores

A `des::queue` holds jobs (events) up to a capacity. **Which** jobs it admits and **in which order** it releases them is decided by its `des::policy`, which also creates the `des::job_store` that actually holds the jobs. The queue and the nodes only use the abstract `job_store` interface, so a new discipline is added by deriving a policy and a store, without changing the library.

```
des::node ──uses──▶ des::queue ──asks admit()──▶ des::policy (shared, holds no jobs)
                        │                              │ make_store()
                        └──────── owns ──────▶ des::job_store (one per queue: container + order)
```

---

## des::queue

**Header:** `libdes_queue.hpp`
**Inherits:** `des::object`

Nodes own one or more queues (waiting queues and server queues). Most queue operations are intentionally used through `des::node` / `des::station`, which are friends of `des::queue`.

### Constructors

```cpp
queue(std::shared_ptr<policy> pol);                          // unlimited positions
queue(int positions, std::shared_ptr<policy> pol);
queue(unsigned int positions, std::shared_ptr<policy> pol);
```

| Parameter | Description |
|---|---|
| `positions` | Maximum capacity (`INT_MAX` = unlimited) |
| `pol` | Queue discipline (e.g. `des::fifo`, `des::is`); must not be null |

Each queue calls `pol->make_store()` once, so the same policy object can be shared by several queues. A queue owns its store and cannot be copied.

### Key Methods

Public:

```cpp
int size() const;
std::string to_string() const;
```

Used internally by nodes and stations:

```cpp
bool   enqueue(const std::shared_ptr<event>& e, double time);   // false if the policy does not admit e
std::shared_ptr<event> dequeue(double time);                    // releases the next job at time `time`
std::shared_ptr<event> dequeue();                               // same, at the next job's time
std::shared_ptr<event> dequeue_next(const std::function<bool(const event&)>& eligible, double time);
bool   has_next(const std::function<bool(const event&)>& eligible) const;
bool   is_full() const;
int    in_queue() const;      // current occupancy
double min_time() const;      // time of the job released next (__DBL_MAX__ if empty)
```

`dequeue_next()` releases the first job, in release order, satisfying `eligible` (used when a freed server may only serve some classes).

### Configuration

```cpp
int         get_positions() const;
void        set_positions(int positions);
std::string get_policy() const;
void        set_policy(policy* pol);     // only on an empty queue
```

### Reset / Clear

```cpp
void reset(double time, std::vector<tag> keys, bool newrun);   // shifts the jobs' times back by `time`
void clear();
```

---

## des::policy (abstract)

**Header:** `libdes_policy.hpp`

```cpp
// Creates the store holding the jobs of one queue that uses this policy.
virtual std::unique_ptr<job_store> make_store() const = 0;

// Tells whether a queue with `positions` places, currently holding `jobs`, admits `e`.
// Default: admit while there is a free place.
virtual bool admit(const event& e, const job_store& jobs, unsigned int positions) const;
```

```cpp
std::string get_description();
void        set_description(std::string d);
```

---

## des::job_store (abstract)

**Header:** `libdes_store.hpp`

The jobs of one queue, released in the order chosen by the policy that created the store. The container and its invariants are private to each implementation.

```cpp
virtual void push(const std::shared_ptr<event>& e, double now) = 0;
virtual const std::shared_ptr<event>& next() const = 0;         // job released next (store not empty)
virtual std::shared_ptr<event> pop(double now) = 0;              // removes next()
virtual std::shared_ptr<event> pop_first(const std::function<bool(const event&)>& eligible, double now) = 0;
virtual bool   has(const std::function<bool(const event&)>& eligible) const = 0;
virtual size_t size() const = 0;
virtual void   for_each(const std::function<void(event&)>& f) = 0;   // may change times; the store restores its order
virtual void   clear() = 0;
```

`now` is the current simulation time; stores whose jobs depend on time (e.g. processor sharing) use it to update the remaining jobs.

### Built-in stores

| Store | Order | Container | Complexity |
|---|---|---|---|
| `des::sequence_store(true)` | earliest time first (FIFO), ties in arrival order | `std::deque` kept in time order | O(1) append for jobs arriving in time order, O(1) release |
| `des::sequence_store(false)` | latest time first (LIFO) | same | same |
| `des::time_store` | earliest time first, ties in arrival order | binary heap in a `std::vector` | O(log n) insert and release |
| `des::ps_store` | earliest departure first, tracked in virtual time | heap on virtual finish tags | O(log n + C) per arrival/departure (C = number of classes) |

None of them allocates per job in steady state.

---

## des::fifo

**Header:** `libdes_fifo.hpp` — store: `sequence_store(true)`

**First-In First-Out**: jobs are released in time order, ties in arrival order. In a waiting queue a job's time is its arrival time; in a server it is its completion time, so a multi-place server releases jobs as they complete.

```cpp
des::fifo();
```

---

## des::is

**Header:** `libdes_is.hpp` — store: `time_store`

**Infinite Server**: every job is admitted regardless of the configured capacity and there is no waiting; jobs leave in order of completion time. Suited to servers holding many jobs at once.

```cpp
des::is();
```

---

## des::ps

**Header:** `libdes_ps.hpp` — store: `ps_store`

Implements **Processor Sharing (PS)** and its weighted generalisation **Generalized Processor Sharing (GPS)**. All admitted jobs share the CPU simultaneously; each class `c` carries a positive weight `w_c` and receives a CPU share proportional to that weight.

With total weight `W = Σ w_{c_k}` over all jobs currently in service, a job of class `c` is served at instantaneous rate `w_c / W`. Setting all weights to `1` (the default) gives standard equal-share PS.

### Constructors

```cpp
des::ps();                                // standard PS — all classes share equally
des::ps(std::vector<double> weights);     // GPS — weights[c] is the weight for class c
```

Classes with index beyond `weights.size()` default to weight `1.0`.

### Virtual time

`des::ps_store` tracks the server in *virtual time* `V`, which advances at rate `1 / W(t)` while jobs are held (and stands still otherwise):

```
dV/dt = 1 / W(t)
```

A job of class `c` arriving at time `a` with service requirement `s` receives the virtual finish tag

```
F = V(a) + s / w_c
```

and departs when `V` reaches `F`: its remaining work at time `t` is `w_c * (F - V(t))`, which decreases at its service rate `w_c / W`. Tags never change after arrival, so the jobs are kept in a heap ordered by `F` (ties in arrival order), and nothing is rescaled when jobs arrive or leave. Between two events `W` is constant, so the next departure happens at

```
t_next = t_V + (F_min - V) * W
```

where `t_V` is the time `V` was last updated. `W` is recomputed from per-class job counts, so it accumulates no rounding error, and `V` restarts from 0 when the server empties. Only the next job's event time is kept up to date; `reset()` brings all of them up to date before shifting them.

### Usage

Attach `des::ps` to the **server** queue of a station with unlimited capacity. Because the server is never full, every arriving job goes directly into service — no separate waiting queue is needed.

```cpp
// Standard PS station (single class, exponential service, unlimited server)
auto mu  = std::make_shared<exponential_distribution<double>>(1.0);
auto ps_policy = std::make_shared<des::ps>();

auto sta = std::make_shared<des::station<double, exponential_distribution>>(
    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{{{mu}}},
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

Derive a policy and return a store from `make_store()`. Override `admit()` only if the admission rule differs from "admit while there is a free place". Nothing else in the library needs to change: pass the policy to a station like the built-in ones.

### Reusing a built-in store: LIFO

```cpp
struct lifo : des::policy
{
    lifo() : des::policy("lifo") {}
    std::unique_ptr<des::job_store> make_store() const override
    {
        return std::make_unique<des::sequence_store>(false);   // release the latest job first
    }
};
```

### Writing a new store: priority

Serve the job with the highest value of a user field first ([tags](tags.md)), ties in arrival order:

```cpp
class priority_store : public des::job_store
{
  public:
    explicit priority_store(des::tag key) : key(key) {}
    void push(const std::shared_ptr<des::event>& e, double) override
    {
        jobs.push_back({e->get_info(key).second, seq++, e});
        std::push_heap(jobs.begin(), jobs.end(), after);
    }
    const std::shared_ptr<des::event>& next() const override { return jobs.front().job; }
    std::shared_ptr<des::event> pop(double) override
    {
        std::pop_heap(jobs.begin(), jobs.end(), after);
        std::shared_ptr<des::event> e = std::move(jobs.back().job);
        jobs.pop_back();
        return e;
    }
    std::shared_ptr<des::event> pop_first(const std::function<bool(const des::event&)>& eligible, double) override
    {
        auto best = jobs.end();
        for(auto it = jobs.begin(); it != jobs.end(); ++it)
            if(eligible(*it->job) && (best == jobs.end() || after(*best, *it))) best = it;
        if(best == jobs.end()) return nullptr;
        std::shared_ptr<des::event> e = std::move(best->job);
        jobs.erase(best);
        std::make_heap(jobs.begin(), jobs.end(), after);
        return e;
    }
    bool has(const std::function<bool(const des::event&)>& eligible) const override
    {
        return std::any_of(jobs.begin(), jobs.end(), [&](const entry& en){ return eligible(*en.job); });
    }
    size_t size() const override { return jobs.size(); }
    void for_each(const std::function<void(des::event&)>& f) override { for(entry& en : jobs) f(*en.job); }
    void clear() override { jobs.clear(); }
  private:
    struct entry { double prio; unsigned long long seq; std::shared_ptr<des::event> job; };
    // heap order: lower priority, or later arrival, goes below
    static bool after(const entry& a, const entry& b) { return a.prio < b.prio || (a.prio == b.prio && a.seq > b.seq); }
    des::tag key;
    std::vector<entry> jobs;
    unsigned long long seq = 0;
};

struct priority : des::policy
{
    explicit priority(des::tag key) : des::policy("priority"), key(key) {}
    std::unique_ptr<des::job_store> make_store() const override { return std::make_unique<priority_store>(key); }
    des::tag key;
};

// use it for the waiting queue of a station
des::tag prio = des::tag_registry::define("priority");
auto sta = std::make_shared<des::station<double, exponential_distribution>>(
    dists, 1, 1, 1, INT_MAX, std::make_shared<priority>(prio), std::make_shared<des::fifo>(), "PRIO", gen);
```

A store whose jobs' departure times depend on the other jobs held (as in processor sharing) can keep its own ordering key, as `ps_store` does with virtual finish tags, and keep the next job's event time up to date: nodes only read the time of `next()`.
