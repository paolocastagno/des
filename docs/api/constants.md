# Constants

**Header:** `libdes_const.hpp`

All string constants used as info-map keys, message keys, and signal names.

---

## Event Info Keys

These keys are set in the `des::event` info map.

| Constant | Value | Description |
|---|---|---|
| `EVENT_ID` | `"event_id"` | Unique event identifier |
| `EVENT_CLS` | `"event_cls"` | Event class index |
| `EVENT_TIME` | `"event_time"` | Scheduled time |
| `EVENT_CONSTRAINT` | `"event_constraint"` | Optional constraint value |
| `EVENT_NODE` | `"event_node"` | Index of the owning node |
| `EVENT_QUEUE` | `"event_queue"` | Index of the queue the event is in |
| `EVENT_SERVER` | `"event_server"` | Index of the server the event is on |
| `EVENT_REROUTE` | `"event_reroute"` | Non-zero if the event was rerouted by the block handler |
| `EVENT_REJECT` | `"event_reject"` | Non-zero if the event was rejected/dropped |

---

## Node Timing Keys

These keys appear in `des::message` payloads sent to observers.

| Constant | Value | Description |
|---|---|---|
| `NODE_ARRIVAL` | `"node_arrival"` | Absolute arrival time at the node |
| `NODE_SERVICE_START` | `"node_service_start"` | Time service began |
| `NODE_SOJOURN` | `"node_sojourn"` | Total time spent in the node (wait + service) |
| `NODE_WAIT` | `"node_wait"` | Time spent waiting in the queue |
| `NODE_SERVICE` | `"node_service"` | Time spent in service |

---

## Signal Names

Passed to `observable::attach()` and `observable::notify()`.

| Constant | Value | Description |
|---|---|---|
| `SIGNAL_NODE_ARRIVAL` | `"signal_node_arrival"` | Fired on event arrival at a node |
| `SIGNAL_NODE_DEPARTURE` | `"signal_node_departure"` | Fired on event departure from a node |
| `SIGNAL_NODE_SERVICE` | `"signal_node_service"` | Fired when an event enters service |
| `SIGNAL_NET_ROUTING` | `"signal_net_routing"` | Fired by the network on routing decisions |

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
| `MESSAGE_KEYVALUE_SEPARATOR` | Separates key from value within a pair |
| `MESSAGE_PAIR_SEPARATOR` | Separates consecutive key-value pairs in a serialised message |
