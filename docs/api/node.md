# des::node and des::observable

## des::observable

**Header:** `libdes_observable.hpp`

Base class that gives an object the ability to notify attached `des::observer` instances when named signals are fired.

### Attachment

```cpp
void attach(std::string signal,
            std::shared_ptr<des::observer> obs);

void detach();                                  // detach all observers
void detach(std::string measure, std::string signal);
```

### Notification (internal)

```cpp
virtual void notify(std::string signal, des::message& msg) = 0;
```

Derived classes call `notify()` to broadcast the signal to every observer registered for it.

### String Identifier

```cpp
std::string get_sid();
void        set_sid(std::string sid);
```

---

## des::node

**Header:** `libdes_node.hpp`
**Inherits:** `des::object`, `des::observable`

`node` is the abstract base class for all simulation stations. It manages a set of waiting queues and server queues, handles arrivals and departures, and fires the standard signals defined in [`libdes_const.hpp`](constants.md).

Concrete subclasses must implement the four protected pure-virtual methods (`get_service`, `schedule`, `enqueue`, `dequeue`). The library provides `des::station`, `des::source`, and `des::sink` as ready-made implementations.

### Constructors

```cpp
node();
node(std::string description);
node(std::string description, std::shared_ptr<std::mt19937_64> g);
node(std::string description, int cls);
node(std::string description, int cls, std::shared_ptr<std::mt19937_64> g);
node(unsigned int cls, std::string description, std::shared_ptr<std::mt19937_64> g);

// Full constructor
node(unsigned int cls,
     std::vector<std::shared_ptr<des::queue>> queues,
     std::vector<std::shared_ptr<des::queue>> servers,
     std::vector<std::vector<int>> queue_map,
     std::vector<std::vector<int>> server_map,
     std::string description,
     std::shared_ptr<std::mt19937_64> g);
```

### Queue Configuration

```cpp
void set_queue(std::vector<std::shared_ptr<queue>>& q);
void set_server(std::vector<std::shared_ptr<queue>>& s);
void set_queue_map(const std::vector<std::vector<int>>& map);
void set_server_map(const std::vector<std::vector<int>>& map);
void set_service_pick_handler(service_pick_handler handler);
```

### State Queries

```cpp
int          queue_length();          // total events waiting
int          queue_length(int cls);   // waiting events of class cls
unsigned int service_length();        // total events in service
unsigned int service_length(unsigned int cls);
```

### Event Processing

```cpp
bool arrival(std::shared_ptr<event>& e);
std::shared_ptr<event> departure();
double next_event_time();             // earliest scheduled departure
```

`arrival()` places the incoming event into a waiting queue, or directly into a server if one is free, and fires `SIGNAL_NODE_ARRIVAL`. It returns `false` when the event cannot be admitted because both service and waiting capacity are unavailable.

`departure()` removes the earliest event from the server queue, fires `SIGNAL_NODE_DEPARTURE`, and optionally pulls the next waiting event into service. If another event enters service, `SIGNAL_NODE_SERVICE` is fired for that event.

### Reset / Clear

```cpp
void reset(double time, std::vector<std::string> keys, bool newrun);
void clear();
```

### Serialisation

```cpp
std::string to_string();
```

### Signals Fired

| Signal constant | Fired when |
|---|---|
| `SIGNAL_NODE_ARRIVAL` | An event arrives at the node |
| `SIGNAL_NODE_DEPARTURE` | An event departs the node |
| `SIGNAL_NODE_SERVICE` | An event enters service |

The `des::message` payload attached to each signal contains the keys `NODE_ARRIVAL`, `NODE_SERVICE_START`, `NODE_SOJOURN`, `NODE_WAIT`, and `NODE_SERVICE` as applicable. See [Constants](constants.md) for the full list.

---

### Protected Pure-Virtual Interface

Subclasses implement these to define how service times are drawn and how events are scheduled/enqueued/dequeued:

```cpp
virtual double get_service(unsigned int& cls, unsigned int& idx) = 0;
virtual int schedule(std::shared_ptr<event>& e,
                     std::vector<std::vector<int>> s_map) = 0;
virtual int enqueue(std::shared_ptr<event>& e,
                    std::vector<std::vector<int>> q_map) = 0;
virtual int dequeue(std::shared_ptr<event>& e,
                    std::vector<std::vector<int>> q_map) = 0;
```

`service_pick_handler` is an optional function pointer used after a departure to choose which waiting queue should supply the next job:

```cpp
using service_pick_handler =
    int (*)(std::shared_ptr<event>,
            int server_idx,
            const std::vector<std::vector<int>>& queue_map,
            const std::vector<std::shared_ptr<queue>>& queues,
            std::shared_ptr<std::mt19937_64>& gen);
```
