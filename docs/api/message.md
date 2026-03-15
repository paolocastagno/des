# des::message

**Header:** `libdes_message.hpp`

A `message` is a lightweight key-value container used as the payload when nodes notify observers. Keys are strings; values are `double`.

---

## Constructors

```cpp
message();
message(std::unordered_map<std::string, double> map);
message(std::string serialised);   // deserialise from string
```

---

## Adding and Removing Entries

```cpp
void add(std::string key, double value);
void add(std::string key);          // inserts key with value 0.0
void remove(std::string key);
void set_keyvalue(std::unordered_map<std::string, double> map);
```

---

## Reading Entries

```cpp
double get_value(std::string key);
```

Returns the value for `key`. Throws (or returns `0`) if the key is absent.

---

## Serialisation

```cpp
std::string serialize();
void        deserialize(std::string msg);
```

The serialised format uses the separators defined in [`libdes_const.hpp`](constants.md):

- `MESSAGE_KEYVALUE_SEPARATOR` separates a key from its value within a pair
- `MESSAGE_PAIR_SEPARATOR` separates one key-value pair from the next

---

## Example

```cpp
des::message msg;
msg.add("sojourn", 3.14);
msg.add(NODE_WAIT,  0.5);

double w = msg.get_value(NODE_WAIT);   // 0.5

std::string s = msg.serialize();
des::message msg2(s);                  // reconstructed from string
```
