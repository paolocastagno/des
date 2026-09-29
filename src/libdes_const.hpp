#ifndef UTILCONST_H
#define UTILCONST_H
#include <string>

#include "libdes_tag.hpp"

using namespace std;

// Event's fields
// Tags of the fields reserved by the library (aliases of des::tags; their registry names
// are given in docs/api/constants.md).
/**
 * @brief Field holding the event id
 * 
 */
inline constexpr des::tag EVENT_ID = des::tags::EVENT_ID;
/**
 * @brief Field holding the event class
 * 
 */
inline constexpr des::tag EVENT_CLS = des::tags::EVENT_CLS;
/**
 * @brief Field holding the time the event will happen (use event::set_time/get_time)
 * 
 */
inline constexpr des::tag EVENT_TIME = des::tags::EVENT_TIME;
/**
 * @brief Field holding the event constraint (use event::set_constraint/get_constraint)
 * 
 */
inline constexpr des::tag EVENT_CONSTRAINT = des::tags::EVENT_CONSTRAINT;
/**
 * @brief Field used to override the routing matrix
 * 
 */
inline constexpr des::tag EVENT_REROUTE = des::tags::EVENT_REROUTE;
/**
 * @brief Field holding the index of the current node
 * 
 */
inline constexpr des::tag EVENT_NODE = des::tags::EVENT_NODE;
/**
 * @brief Field holding the index of the queue in the current node
 * 
 */
inline constexpr des::tag EVENT_QUEUE = des::tags::EVENT_QUEUE;
/**
 * @brief Field holding the index of the server in the current node
 * 
 */
inline constexpr des::tag EVENT_SERVER = des::tags::EVENT_SERVER;
/**
 * @brief Field set to 1 when the event was refused by a destination
 * 
 */
inline constexpr des::tag EVENT_REJECT = des::tags::EVENT_REJECT;
/**
 * @brief Field holding the arrival time at the current node
 * 
 */
inline constexpr des::tag NODE_ARRIVAL = des::tags::NODE_ARRIVAL;
/**
 * @brief Field holding the time at which service started in the current node
 * 
 */
inline constexpr des::tag NODE_SERVICE_START = des::tags::NODE_SERVICE_START;
/**
 * @brief Field holding the time spent in the last node visited (wait + service)
 * 
 */
inline constexpr des::tag NODE_SOJOURN = des::tags::NODE_SOJOURN;
/**
 * @brief Field holding the waiting time experienced in the last node visited
 * 
 */
inline constexpr des::tag NODE_WAIT = des::tags::NODE_WAIT;
/**
 * @brief Field holding the service time experienced in the last node visited
 * 
 */
inline constexpr des::tag NODE_SERVICE = des::tags::NODE_SERVICE;

/**
 * @brief Signal: arrival in the node
 * 
 */
const string SIGNAL_NODE_ARRIVAL = "node_arrival";

/**
 * @brief Signal: departure from the node
 *
 */
const string SIGNAL_NODE_DEPARTURE = "node_departure";
/**
 * @brief Signal: job enters service in the node
 *
 */
const string SIGNAL_NODE_SERVICE = "node_service";
/**
 * @brief Signal: departure from the node
 * 
 */
const string SIGNAL_NET_ROUTING = "net_route";
/**
 * @brief Signal prefix: per-run throughput of an edge, fed once per run by network::reset()
 *
 */
const string SIGNAL_NET_FLOW = "net_flow";
/**
 * @brief Signal prefix: an event was refused by the destination of an edge
 *
 */
const string SIGNAL_NET_BLOCK = "net_block";
/**
 * @brief Signal prefix: an event departing a node was lost because no destination accepted it
 *
 */
const string SIGNAL_NET_LOSS = "net_loss";
/**
 * @brief Separator used between key and value
 * 
 */
const string MESSAGE_KEYVALUE_SEPARATOR = ",";
/**
 * @brief separator used between two subsequent key-value pairs
 * 
 */
const string MESSAGE_PAIR_SEPARATOR = ";";

// Constraint's constants
const string L = "<";
const string G = ">";
const string LEQ = "<=";
const string GEQ = ">=";
const string EQ = "=";
const string NEQ = "!=";

// Network's constant
#endif