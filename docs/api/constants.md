# Constants

**Header:** `libdes_const.hpp`

The fields reserved by the library and the signal names.

The event info and node timing fields are [tags](tags.md) (`des::tag`), aliases of the predefined `des::tags` with the same identifier (e.g. `NODE_ARRIVAL` is `des::tags::NODE_ARRIVAL`). The *Name* column is the name each tag has in `des::tag_registry`, used by `message::serialize()` and `event::to_string()`; these names are reserved, so `des::tag_registry::define()` rejects them.

---

## Event Info Keys

These fields are set in the `des::event` info fields.

| Constant | Name | Description |
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

These fields appear in `des::message` payloads sent to observers.

| Constant | Name | Description |
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
| `SIGNAL_NET_ROUTING` | `"net_route"` | Prefix for network routing signals (`net_route_<src>_<dst>`), fired when an event is accepted by `dst` |
| `SIGNAL_NET_FLOW` | `"net_flow"` | Prefix for per-edge throughput signals (`net_flow_<src>_<dst>`), fed once per run by `network::reset()` |
| `SIGNAL_NET_BLOCK` | `"net_block"` | Prefix for per-edge blocking signals (`net_block_<src>_<dst>`), fired when `dst` refuses an event |
| `SIGNAL_NET_LOSS` | `"net_loss"` | Prefix for per-node loss signals (`net_loss_<src>`), fired when an event leaving `src` is dropped because no destination accepted it |

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
