# des::message

**Header:** `libdes_message.hpp`

A `message` is a lightweight container of fields used as the payload when nodes notify observers. Fields are addressed by [tag](tags.md); values are `double`.

Nodes and the network notify observers with a *view* of the event's fields (`message::view_of(e->get_store())`), so no data is copied. A copy of a view message owns its data, and adding or removing a field makes a view take its own copy first, so observers may safely keep messages.

---

## Constructors

```cpp
message();                                         // empty
static message view_of(const tag_store& fields);   // no copy; fields must outlive the message
```

---

## Adding and Removing Fields

```cpp
void add(tag t, double value);     // replaces any previous value
void remove(tag t);
```

---

## Reading Fields

```cpp
double get_value(tag t) const;     // 0 if the field is absent
bool   has(tag t) const;
```

---

## Serialisation

```cpp
std::string serialize() const;
```

Writes the fields as text, e.g. for logging, naming each one after `des::tag_registry::name()`. The format uses the separators defined in [`libdes_const.hpp`](constants.md):

- `MESSAGE_KEYVALUE_SEPARATOR` separates a name from its value within a pair
- `MESSAGE_PAIR_SEPARATOR` separates one pair from the next

---

## Example

```cpp
const des::tag response = des::tag_registry::define("response_time");

des::message msg;
msg.add(response, 3.14);
msg.add(NODE_WAIT, 0.5);

double w = msg.get_value(NODE_WAIT);   // 0.5
std::string s = msg.serialize();       // "node_wait,0.500000;response_time,3.140000;"
```
