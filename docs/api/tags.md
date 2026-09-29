# Tags

**Header:** `libdes_tag.hpp`

Events and messages store their information fields in a vector indexed by **tags**: small integer identifiers obtained once, at setup time, instead of hashing a string on every access.

---

## des::tag and des::tags

```cpp
struct tag { unsigned int id; };
```

The fields reserved by the library have predefined tags in `des::tags`; [`libdes_const.hpp`](constants.md) aliases them with the same identifiers at global scope (e.g. `NODE_SOJOURN`). Their registry names:

| Tag | Name |
|---|---|
| `tags::EVENT_ID` | `"id"` |
| `tags::EVENT_CLS` | `"class"` |
| `tags::EVENT_TIME` | `"time"` |
| `tags::EVENT_CONSTRAINT` | `"constraint"` |
| `tags::EVENT_REROUTE` | `"reroute"` |
| `tags::EVENT_NODE` | `"node"` |
| `tags::EVENT_QUEUE` | `"queue_idx"` |
| `tags::EVENT_SERVER` | `"server_idx"` |
| `tags::EVENT_REJECT` | `"reject"` |
| `tags::NODE_ARRIVAL` | `"arrival_time"` |
| `tags::NODE_SERVICE_START` | `"service_start_time"` |
| `tags::NODE_SOJOURN` | `"node_sojourn"` |
| `tags::NODE_WAIT` | `"node_wait"` |
| `tags::NODE_SERVICE` | `"node_service"` |

---

## des::tag_registry

A process-wide, thread-safe mapping between names and tags.

```cpp
static tag define(const std::string& name);              // user field; same name -> same tag
static std::pair<bool, tag> find(const std::string& name);   // look up any name, reserved ones included
static std::string name(tag t);
static unsigned int size();
static constexpr bool is_builtin(tag t);
```

`define()` throws `std::invalid_argument` for a reserved name, so a user field can never silently alias a library field. Names are only used at setup time (`define()`, `find()`, e.g. when reading a model description) and for output (`name()`, used by `message::serialize()` and `event::to_string()`): events, messages and observers are addressed by tag only.

---

## des::tag_store

The tag-indexed values of an event (`event::get_store()`) or of a message.

```cpp
bool has(tag t) const;
std::pair<bool,double> get(tag t) const;   // (false, 0) if absent
double value(tag t) const;                 // 0 if absent
void set(tag t, double v);
bool insert(tag t, double v);              // only if absent
void remove(tag t);
void clear();
template <typename F> void for_each(F f) const;   // f(tag, value), in tag order
```

---

## Using tags

Define each user field once, at model setup, and keep the tag:

```cpp
// model setup
const des::tag priority = des::tag_registry::define("priority");

// per event
e->emplace_info(priority, 2);
double p = e->get_info(priority).second;

// custom observer
void update(const des::message& msg) override
{
    double p = msg.get_value(priority);
    int  cls = static_cast<int>(msg.get_value(des::tags::EVENT_CLS));
}
```
