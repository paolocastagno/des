# des::event

**Header:** `libdes_event.hpp`
**Inherits:** `des::object`

An `event` represents a single discrete occurrence in the simulation. It carries a scheduled time, an event class, an optional constraint, and an open-ended key-value info map for user-defined metadata.

---

## Constructors

```cpp
event();
event(int cls);
event(int cls, std::unordered_map<std::string, double> info);
```

| Parameter | Description |
|---|---|
| `cls` | Integer event class (default `0`) |
| `info` | Initial key-value metadata |

---

## Time

```cpp
void   set_time(double time);
double get_time();          // throws if not initialised
bool   is_initialized();
```

An event is considered *uninitialised* until `set_time()` has been called at least once.

---

## Class

```cpp
void set_cls(int cls);
int  get_cls();
```

The class is an integer label used to distinguish event types (e.g. different traffic classes).

---

## Constraint

```cpp
void   set_constraint(double cons);
double get_constraint();
```

An optional constraint value. Its meaning is user-defined; the library does not enforce it automatically.

---

## Info Map

The info map stores arbitrary `double` values keyed by string. Standard keys are defined in [`libdes_const.hpp`](constants.md).

```cpp
void   set_info(std::string name, double val);
double get_info(std::string name);            // returns {found, value}
void   emplace_info(std::string name, double val);  // replace existing
void   remove_info(std::string name);
std::unordered_map<std::string, double> get_map_info();
```

The important info key used by the network is `EVENT_NODE`, which holds the index of the node that currently owns the event.

---

## Cloning and Reset

```cpp
void clone(event e);
void reset(double time, std::vector<std::string> keys, bool newrun);
void clear();
```

`reset()` resets the event time to `time` and clears any info keys listed in `keys`.

---

## Comparison Operators

Events are ordered by time, which allows them to be sorted in priority queues:

```cpp
bool operator<(const event& rhs) const;
bool operator>(const event& rhs) const;
bool operator<=(const event& rhs) const;
bool operator>=(const event& rhs) const;
bool operator==(const event& rhs) const;
bool operator!=(const event& rhs) const;
```

---

## Serialisation

```cpp
std::string to_string();
```

Returns a human-readable representation of the event's state.

---

## Example

```cpp
auto e = std::make_shared<des::event>();
e->set_cls(0);
e->set_time(3.14);
e->set_info(EVENT_NODE, 0);   // owned by node 0

double t = e->get_time();       // 3.14
int    c = e->get_cls();        // 0
```
