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
virtual void update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    unsigned int positions,
                    double time) = 0;

virtual void update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    unsigned int positions) = 0;

virtual void update(std::shared_ptr<event> e,
                    std::list<std::shared_ptr<event>>& l,
                    double time) = 0;

virtual std::shared_ptr<event> front() = 0;
```

`update()` inserts `e` into the list `l` according to the discipline's ordering rules. `front()` returns the next event to be dequeued without removing it.

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

## Implementing a Custom Policy

Subclass `des::policy` and implement the three `update()` overloads and `front()`:

```cpp
class my_lifo : public des::policy {
public:
    void update(std::shared_ptr<des::event> e,
                std::list<std::shared_ptr<des::event>>& l,
                unsigned int, double) override
    {
        l.push_front(e);   // LIFO: insert at front
    }
    // ... other overloads and front()
};
```
