# Observers

Observers implement the measurement layer. They are attached to an `observable` node on a named signal and receive a `des::message` each time that signal fires. The library ships five concrete observer types: four measure the notified events, and `des::ratio` is fed directly with pairs of values, e.g. from the regeneration cycles of a run.

`scalar`, `sample` and `histogram` measure one field of the notified event, given as a [tag](tags.md) when they are built (e.g. `NODE_SOJOURN`, or a tag from `des::tag_registry::define()`); they are then named after that field. Built with a description instead, they measure nothing by themselves and are fed directly with `update(value, cls)`; notifying them a message throws `std::logic_error`.

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

virtual void update(const des::message& msg) = 0;   // fields addressed by des::tag
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
scalar(des::tag field, int n_classes);                 // measures `field`, named after it
scalar(const std::string& description, int n_classes); // fed with update(value, cls) only
```

### Updating

```cpp
void update(const des::message& msg);   // reads the field and EVENT_CLS
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
void update(const des::message& msg);   // increments the class EVENT_CLS
void update(int cls);                   // increment class cls by 1
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
sample(des::tag field, int n_classes);                 // stores `field`, named after it
sample(const std::string& description, int n_classes); // fed with update(value, cls) only
```

### Updating

```cpp
void update(const des::message& msg);
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
histogram(des::tag field, int n_classes);                               // bins `field`, named after it
histogram(des::tag field, const std::string& event, int n_classes);
histogram(const std::string& description, int n_classes);               // fed with update(value, cls) only
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
void update(const des::message& msg);
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

## des::ratio

**Header:** `libdes_ratio.hpp`

Estimates a ratio of sums, r = Σx / Σy, from independent pairs (x, y). Its typical use is the **regenerative method**: the cycles between two regeneration points of a model are independent and identically distributed, and each contributes one pair. The pair can be, for example, the jobs a station served in the cycle and the cycle's length, which gives the throughput. It can also be the sojourn times summed over the cycle and the jobs served in it, which gives the mean sojourn time.

Because x and y are correlated (longer cycles serve more jobs), the variance of r depends on their covariance, which a pair of `scalar`s cannot provide. The ratio keeps the means and co-moments of x and y with Welford's online algorithm. The confidence interval of the current run is

r ± t<sub>1−α/2, k−1</sub> · s / (ȳ · √k),  with s² = Σ(x − r·y)² / (k − 1),

where k is the number of pairs and ȳ = Σy / k.

A ratio reads no message field: it is fed with `update(x, y, cls)` only, and notifying it a message throws `std::logic_error`.

### Constructor

```cpp
ratio(const std::string& description, int n_classes);
```

### Updating

```cpp
void update(double x, double y, int cls);   // add one pair to the current run of class cls
```

### Reading

```cpp
double get(int cls);                // current-run estimate Σx / Σy (0 while Σy is 0)
long   n_updates(int cls);          // pairs in the current run
std::pair<double,double> run_confidence_interval(double alpha, int cls);   // within the current run
double get_ratio(int cls);          // mean of the completed-run estimates
size_t completed_runs(int cls);
std::pair<double,double> confidence_interval(double alpha, int cls);       // across completed runs
std::vector<std::pair<double,double>> confidence_interval(double alpha);
```

Both intervals are unbounded, `[-DBL_MAX, DBL_MAX]`, until there are at least two pairs (within a run) or two completed runs (across runs).

### Reset

```cpp
void reset(bool newrun);            // newrun=true stores each class's current-run estimate
void reset(int cls, bool newrun);
void clear();                       // discard the current run and the completed-run estimates
```

A ratio is not attached to a node, so `network::reset()` does not reach it: call `reset(true)` at the end of each run. [A first model](../../README.md#a-first-model) ends each run at a regeneration point once `run_confidence_interval()` is narrow enough, then adds runs until `confidence_interval()` is.

---

## Attaching an Observer

```cpp
auto sojourn = std::make_shared<des::scalar>(NODE_SOJOURN, 1);
myNode->attach(SIGNAL_NODE_DEPARTURE, sojourn);
```

After `attach()` the observer's `update(const des::message&)` method is called automatically every time `myNode` fires `SIGNAL_NODE_DEPARTURE`.
