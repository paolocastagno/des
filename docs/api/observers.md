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
unsigned int get_id();              // unique numeric ID

void set_event(std::string signal);

virtual void update(std::string message) = 0;
virtual void update(const des::message& msg);
virtual void reset(bool newrun = false)          = 0;
virtual void reset(int cls, bool newrun = false) = 0;
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
void update(const des::message& msg);   // update directly from message object
void update(double value, int cls);     // direct update for class cls
```

### Reading

```cpp
double get(int cls);            // mean for the current run
double mean(int cls);           // alias for get()
double get_scalar(int cls);     // mean of means across all completed runs
double run_avg(int cls);        // deprecated alias for get()
double run_stddev(int cls);     // sample std dev within the current run
double stddev(int cls);         // std dev across completed run means
size_t completed_runs(int cls);
int    n_updates(int cls);
std::pair<double,double> confidence_interval(double alpha, int cls);
std::vector<std::pair<double,double>> confidence_interval(double alpha);
```

### Reset

```cpp
void reset(bool newrun);          // prepare for a new run
void reset(int cls, bool newrun); // same, for a single class
void clear();                     // discard current and completed-run data
```

Use `reset(true)` at the end of a replication to store the current-run mean for cross-run confidence intervals.

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
void update(const des::message& msg);
void update(int cls);               // increment class cls by 1
```

### Reading

```cpp
double get(int cls);
std::pair<double,double> confidence_interval(double alpha, int cls);
```

### Reset / Clear

```cpp
void reset(bool newrun);
void reset(int cls, bool newrun);
void clear();                     // clears current counters; completed-run data is preserved
```

---

## des::sample

**Header:** `libdes_sample.hpp`

Stores every individual observation, enabling post-hoc analysis.
Note: memory usage grows linearly with the number of events.

### Constructor

```cpp
sample(const std::string& description, int n_classes);
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
unsigned int observations(int cls);    // number of stored samples
std::vector<std::vector<double>> get();  // all observations [cls][i]
std::pair<double,double> confidence_interval(double alpha, int cls);
std::vector<std::pair<double,double>> confidence_interval(double alpha);
```

### Reset / Clear

```cpp
void reset(bool newrun = false);
void reset(int cls, bool newrun);
void clear();                     // clears current samples; completed-run data is preserved
```

For `des::sample`, `reset(true)` stores the current run's per-class sums for confidence-interval calculations.

---

## des::histogram

**Header:** `libdes_histogram.hpp`

Bins observations into equal-width buckets. Useful for visualising service-time or inter-arrival-time distributions.

### Constructor

```cpp
histogram(const std::string& description, int n_classes);
histogram(const std::string& description, const std::string& event, int n_classes);
```

### Configuration

```cpp
bool set_binsize(double bin_width);
```

Returns `true` when the bin size was changed. It must be called before any current-run buckets exist.

### Updating

```cpp
void update(double value, int cls);
void update(std::string message);
```

### Run Management

```cpp
void end_run();
void end_run(double time);
void reset(double time, bool newrun);
```

Call at the end of each replication to store the current histogram before `reset()`.

### Statistics

```cpp
double mean(int cls);
double stddev(int cls);
unsigned int observations(int cls);
std::vector<std::vector<des::histogram::bin>> get();
std::vector<std::vector<double>> confidence_interval(double alpha, int cls);
std::vector<std::vector<std::vector<double>>> confidence_interval(double alpha);
```

### Output

```cpp
std::string print(double alpha = 1e-2);  // CSV-friendly tabular output
std::string to_string();
```

### Reset / Clear

```cpp
void reset(bool newrun);          // newrun=true stores the current buckets as a completed run
void reset(int cls, bool newrun);
void clear();                     // clears current buckets; completed-run data is preserved
```

---

## Attaching an Observer

```cpp
auto thr = std::make_shared<des::scalar>("throughput", 1);
myNode->attach(SIGNAL_NODE_DEPARTURE, thr);
```

After `attach()` the observer's `update(const des::message&)` method is called automatically every time `myNode` fires `SIGNAL_NODE_DEPARTURE`.
