# Observers

Observers implement the measurement layer. They are attached to an `observable` node on a named signal and receive a `des::message` each time that signal fires. The library ships four concrete observer types.

All observers inherit from `des::observer`.

---

## des::observer (abstract base)

**Header:** `libdes_observer.hpp`

```cpp
bool        get_attached();
void        set_attached(bool b);
std::string get_observer();         // observer name / description
std::string get_event();            // signal this observer listens to
int         get_id();               // unique numeric ID

void set_event(std::string signal);

virtual void update(std::string message) = 0;
virtual void reset()                     = 0;
virtual void reset(int cls)              = 0;
virtual void clear()                     = 0;
virtual std::string to_string()          = 0;
```

---

## des::scalar

**Header:** `libdes_scalar.hpp`

Collects a running mean and variance for a single numeric quantity, supporting multi-run aggregation and confidence intervals.

### Constructor

```cpp
scalar(const std::string& description, int n_classes);
```

### Updating

```cpp
void update(std::string message);       // parse value from message key-value string
void update(double value, int cls);     // direct update for class cls
```

### Reading

```cpp
double get(int cls);            // mean for the current run
double get_scalar(int cls);     // mean of means across all completed runs
double run_avg(int cls);        // alias for get_scalar
double stddev(int cls);         // std dev across runs
std::pair<double,double> confidence_interval(double alpha, int cls);
```

### Reset

```cpp
void reset();           // prepare for a new run (stores current run result)
void reset(int cls);    // same, for a single class
void clear();           // discard all data
```

---

## des::counter

**Header:** `libdes_counter.hpp`

Counts event occurrences per class.

### Constructor

```cpp
counter(const std::string& description, int n_classes);
```

### Updating

```cpp
void update(std::string message);   // increment from message
void update(int cls);               // increment class cls by 1
```

### Reading

```cpp
double get(int cls);
std::pair<double,double> confidence_interval(double alpha, int cls);
```

### Reset / Clear

```cpp
void reset();
void reset(int cls);
void clear();
```

---

## des::vector

**Header:** `libdes_vector.hpp`

Stores every individual observation, enabling post-hoc analysis.
Note: memory usage grows linearly with the number of events.

### Constructor

```cpp
vector(const std::string& description, int n_classes);
```

### Updating

```cpp
void update(std::string message);
void update(double value, int cls);
```

### Statistics

```cpp
double mean(int cls);
double stddev(int cls);
int    observations(int cls);    // number of stored samples
std::vector<std::vector<double>> get();  // all observations [cls][i]
std::pair<double,double> confidence_interval(double alpha, int cls);
```

### Reset / Clear

```cpp
void reset();
void reset(int cls);
void clear();
```

---

## des::histogram

**Header:** `libdes_histogram.hpp`

Bins observations into equal-width buckets. Useful for visualising service-time or inter-arrival-time distributions.

### Constructor

```cpp
histogram(const std::string& description, int n_classes);
```

### Configuration

```cpp
void set_binsize(double bin_width);
```

Must be called before any `update()`.

### Updating

```cpp
void update(double value, int cls);
void update(std::string message);
```

### Run Management

```cpp
void end_run();
void end_run(double time);
```

Call at the end of each replication to store the current histogram before `reset()`.

### Statistics

```cpp
double mean(int cls);
double stddev(int cls);
int    observations(int cls);
std::map<double, des::histogram::bin> get();  // bucket map
std::pair<double,double> confidence_interval(double alpha, int cls);
```

### Output

```cpp
std::string print();       // CSV-friendly tabular output
std::string to_string();
```

### Reset / Clear

```cpp
void reset(bool newrun = false);   // newrun=true preserves cross-run data
void clear();
```

---

## Attaching an Observer

```cpp
auto thr = std::make_shared<des::scalar>("throughput", 1);
myNode->attach("throughput", SIGNAL_NODE_DEPARTURE, thr);
```

After `attach()` the observer's `update(string)` method is called automatically every time `myNode` fires `SIGNAL_NODE_DEPARTURE`.
