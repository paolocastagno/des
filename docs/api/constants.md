# Constants

**Header:** `libdes_const.hpp`

All string constants used as info-map keys, message keys, and signal names.

---

## Event Info Keys

These keys are set in the `des::event` info map.

| Constant | Value | Description |
|---|---|---|
| `EVENT_ID` | `"id"` | Unique event identifier |
| `EVENT_CLS` | `"class"` | Event class index |
| `EVENT_TIME` | `"time"` | Scheduled time |
| `EVENT_CONSTRAINT` | `"constraint"` | Optional constraint value |
| `EVENT_NODE` | `"node"` | Index of the owning node |
| `EVENT_QUEUE` | `"queue_idx"` | Index of the queue the event is in |
| `EVENT_SERVER` | `"server_idx"` | Index of the server the event is on |
| `EVENT_REROUTE` | `"reroute"` | Reserved key for custom rerouting logic |
| `EVENT_REJECT` | `"reject"` | Non-zero if the event was rejected/dropped |

---

## Node Timing Keys

These keys appear in `des::message` payloads sent to observers.

| Constant | Value | Description |
|---|---|---|
| `NODE_ARRIVAL` | `"arrival_time"` | Absolute arrival time at the node |
| `NODE_SERVICE_START` | `"service_start_time"` | Time service began |
| `NODE_SOJOURN` | `"node_sojourn"` | Total time spent in the node (wait + service) |
| `NODE_WAIT` | `"node_wait"` | Time spent waiting in the queue |
| `NODE_SERVICE` | `"node_service"` | Time spent in service |

---

## Signal Names

Passed to `observable::attach()` and `observable::notify()`.

| Constant | Value | Description |
|---|---|---|
| `SIGNAL_NODE_ARRIVAL` | `"node_arrival"` | Fired on event arrival at a node |
| `SIGNAL_NODE_DEPARTURE` | `"node_departure"` | Fired on event departure from a node |
| `SIGNAL_NODE_SERVICE` | `"node_service"` | Fired when an event enters service |
| `SIGNAL_NET_ROUTING` | `"net_route"` | Prefix for network routing signals |

---

## Constraint Operator Tokens

Used in the constraint field of an event to express relational conditions.

| Constant | Meaning |
|---|---|
| `L` | Less than |
| `G` | Greater than |
| `LEQ` | Less than or equal |
| `GEQ` | Greater than or equal |
| `EQ` | Equal |
| `NEQ` | Not equal |

---

## Message Format Separators

| Constant | Description |
|---|---|
| `MESSAGE_KEYVALUE_SEPARATOR` | `","`; separates key from value within a pair |
| `MESSAGE_PAIR_SEPARATOR` | `";"`; separates consecutive key-value pairs in a serialised message |
