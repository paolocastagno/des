#ifndef UTILCONST_H
#define UTILCONST_H
#include <string>

using namespace std;

// Event's constants
/**
 * @brief Key used to lookup for the event id in the data structure Event::info 
 * 
 */
const string EVENT_ID = "id";
/**
 * @brief Key used to lookup for the event class in the data structure Event::info 
 * 
 */
const string EVENT_CLS = "class";
/**
 * @brief Key used to lookup for the event end time in the data structure Event::info 
 * 
 */
const string EVENT_TIME = "time";

/**
 * @brief Key used to lookup for the event end constraint in the data structure Event::info 
 * 
 */
const string EVENT_CONSTRAINT = "constraint";

/**
 * @brief Key used to override the routing matrix
 * 
 */
const string EVENT_REROUTE = "reroute";

/**
 * @brief Key used to lookup the int id (or index) of the current node  
 * 
 */
const string EVENT_NODE = "node";

/**
 * @brief Key used to lookup the index of the queue in the current node  
 * 
 */
const string EVENT_QUEUE = "queue_idx";

/**
 * @brief Key used to lookup the index of the queue in the current node  
 * 
 */
const string EVENT_SERVER = "server_idx";

/**
 * @brief Key with used to lookup the int id (or index) of the current node  
 * 
 */
const string EVENT_REJECT = "reject";

/**
 * @brief Key used to lookup the arrival time at the current node  
 * 
 */
const string NODE_ARRIVAL = "arrival_time";

/**
 * @brief Key used to lookup the arrival time at the current node  
 * 
 */
const string NODE_SOJOURN = "node_sojourn";

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
 * @brief Signal: departure from the node
 * 
 */
const string SIGNAL_NET_ROUTING = "net_route";
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