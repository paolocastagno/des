# libdes

**libdes** is a C++ library for discrete-event simulation of queueing networks. It is built for performance studies of systems in which jobs compete for servers, such as computer and communication systems, production lines and service centres.

You build a model from C++ objects: sources that generate jobs, stations that queue and serve them, sinks that absorb them, and a routing matrix that moves them from node to node, class by class. The library runs the event list, the queues and their service disciplines, the routing and the random number streams, and it collects the statistics you ask for. Your program keeps the simulation loop, so you decide the run length, the warm-up and the number of replications. The results are point estimates with confidence intervals. Anything a study is likely to change, such as a queue discipline, a routing rule or a statistic, is a class or a function you can replace without modifying the library.

## Features

### Models

- **Open and closed networks with multiple job classes.** Each class has its own arrival process, service times and route.
- **General arrival and service times.** Sources and stations are templates on a `<random>` distribution, including exponential, gamma (Erlang), Weibull, lognormal, uniform and empirical (piecewise constant). Each class, and each server, can have its own parameters.
- **Multi-server stations** can have several waiting queues, each with finite or unlimited capacity. Maps set which queues and servers each class may use.
- **Service disciplines.** FIFO, infinite server, processor sharing and weighted generalized processor sharing (GPS) are built in. A new discipline is a policy plus a job store, and adding one does not change the library. The documentation shows how to write LIFO and priority queues.
- **Blocking and custom routing.** A full node refuses new jobs. A refused job is lost, unless a block handler reroutes it. A routing handler can replace the matrix with your own rule. A service-pick handler chooses which waiting queue a freed server serves next, for example to give one class strict priority.

### Measurement and output analysis

- **Observers** attach to node signals (arrival, start of service, departure) and measure one event field, class by class. The field can be the sojourn, waiting or service time, or one you define. `scalar` keeps the mean and variance, `counter` counts events, `sample` keeps every observation and `histogram` bins them. `ratio` estimates Σx / Σy from independent pairs, such as the regeneration cycles of a run, with a confidence interval that accounts for the covariance of x and y.
- **Flow statistics** are collected by every network, with no setup: throughput and blocked jobs on each edge, and lost jobs at each node.
- **Replications and confidence intervals.** `reset()` closes a run and either keeps its statistics or, after a warm-up period, discards them. Independent replications give a Student-t confidence interval for every measure. Runs that start and end at regeneration points are independent and need no warm-up.

### Random numbers

- **A separate stream for every source of randomness.** One seed drives a `xoshiro256**` generator, which is split by jump-ahead into disjoint streams. Each node gets one stream per class for its inter-arrival or service times, and one for its choices among queues and servers. The routing out of each node gets one stream per class.
- **Common random numbers.** A stream depends only on the node's index, not on the rest of the model. Two variants of a model run with the same seed therefore draw the same random numbers wherever they coincide, which lowers the variance of the difference between them.

### Performance and testing

- **Built for long runs.** Event fields are addressed by integer tags rather than strings. The next event comes from an indexed heap over the nodes. Events are recycled through a pool, and the job stores do not allocate memory per job in steady state. `make bench` measures events per second on your machine.
- **Checked against theory.** Besides unit and regression tests, the suite compares simulated estimates with exact results: M/M/1, Erlang B and C, Pollaczek–Khinchine, processor-sharing insensitivity, a Jackson tandem, and mean value analysis of a closed network. The tests also run under AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer. CI runs all of this on Linux and macOS on every push. See [Testing](docs/testing.md).

## How it works

![libdes architecture: your program seeds and drives the event loop of a des::network, here a central server model in which jobs alternate between a processor-sharing CPU and a FIFO disk before leaving through a sink; every node draws from its own block of random streams; every queue delegates to a policy and the job store it creates; observers attached to node signals collect the statistics](figures/architecture.svg)

Your program builds the network with a seed and drives the simulation loop. The figure shows a central server model. Jobs of two classes share a processor-sharing CPU. After each CPU burst, a job of class c visits a FIFO disk with probability p[c] and then returns to the CPU; otherwise it leaves. `next_event()` asks the network's heap for the node whose next job departs first (1), and that job departs (2). `route()` samples the next node from the routing matrix (3) and hands the job over (4). A station starts the job on a free server or puts it in a waiting queue, and the sink absorbs it at once. Each queue leaves admission and release order to its policy and to the job store the policy creates: a ring buffer for FIFO, a binary heap for infinite server, and a heap in virtual time for processor sharing. Every node draws from its own block of random streams, and so does the routing out of it. Observers attached to node signals collect the statistics. The dashed parts are the ones you can replace with your own. [Architecture](docs/architecture.md) covers the design in detail.

## Quick start

### Requirements

- A C++23 compiler (C++20 also works with `make CXXSTD=c++20`)
- GNU Make
- macOS or Linux

### Build and run the example

```sh
git clone https://github.com/paolocastagno/des.git
cd des
make                          # builds libdes.dylib (macOS) or libdes.so (Linux)
make test LINK_INSTALLED=0    # builds and runs test/test against the library just built
```

[`test/test.cpp`](test/test.cpp) simulates an M/M/1 and an M/M/2 queue with the stopping rules of the first model below. It prints confidence intervals for throughput and mean sojourn time next to the values from queueing theory. `make test` links against the installed library by default. `LINK_INSTALLED=0` uses the one in the working tree instead.

### A first model

An M/M/1 queue with arrival rate 0.8 and service rate 1, estimated with two stopping rules built on regeneration points. A regeneration point is a departure that leaves the station empty. Arrivals are Poisson, so at that point the model is back in its initial state, and what follows is independent of what came before.

- **Within a run.** A run is a sequence of regeneration cycles, and the cycles are independent. Each cycle gives a `des::ratio` observer one pair: the jobs the station served in the cycle and the cycle's length. The observer returns the run's throughput with a 95% confidence interval (the regenerative method). The run ends at the first regeneration point at which that interval is within 5% of the estimate.
- **Across runs.** Every run starts and ends at a regeneration point, so the runs are independent and identically distributed, and none needs a warm-up period. The program adds runs until the 95% confidence interval across runs is within 1% of its mean.

```cpp
#include <climits>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_network.hpp>
#include <libdes_ratio.hpp>
#include <libdes_scalar.hpp>
#include <libdes_sink.hpp>
#include <libdes_source.hpp>
#include <libdes_station.hpp>

using namespace std;

int main()
{
    // Poisson arrivals (rate 0.8) -> one exponential server (rate 1) -> sink
    auto arr = make_shared<exponential_distribution<double>>(0.8);
    auto src = make_shared<des::source<double, exponential_distribution>>(
        vector<shared_ptr<exponential_distribution<double>>>{arr},   // [class]
        "Source");
    auto svc = make_shared<exponential_distribution<double>>(1.0);
    auto sta = make_shared<des::station<double, exponential_distribution>>(
        vector<vector<shared_ptr<exponential_distribution<double>>>>{{svc}},   // [server][class]
        1, 1,          // 1 server, 1 job per server
        1, INT_MAX,    // 1 waiting queue, unlimited capacity
        "M/M/1");
    auto snk = make_shared<des::sink>("Sink");

    // Measure the time jobs spend in the station
    auto sojourn = make_shared<des::scalar>(NODE_SOJOURN, 1);
    sta->attach(SIGNAL_NODE_DEPARTURE, sojourn);

    // Measure the station's throughput over regeneration cycles: jobs served / cycle length
    auto throughput = make_shared<des::ratio>("throughput", 1);

    // routing[from][to][class]: source -> station -> sink
    vector<vector<vector<double>>> routing = {
        {{0}, {1}, {0}},
        {{0}, {0}, {1}},
        {{0}, {0}, {0}}
    };
    des::network net({src, sta, snk}, routing, 42);   // seed of every node's random streams

    // Bootstrap: one event at the source, which then schedules every arrival
    auto e = make_shared<des::event>();
    e->set_cls(0);
    e->set_time(0.0);
    e->set_info(EVENT_NODE, 0);
    src->arrival(e);

    const double alpha = 0.05;           // 95% confidence intervals
    const double tolerance = 0.01;       // across runs: stop when the throughput CI is within 1% of its mean
    const double run_tolerance = 0.05;   // within a run: end it when its own throughput CI is within 5%
    const int min_runs = 10;             // runs before the first check across runs
    const int min_cycles = 30;           // cycles before the first check within a run

    // An interval is narrow enough when its half-width is within tol times its midpoint
    auto narrow = [](pair<double, double> ci, double tol) { return ci.second - ci.first <= tol * (ci.first + ci.second); };

    do
    {
        // A run is a sequence of regeneration cycles. A cycle ends at a departure that
        // leaves the station empty: arrivals are Poisson, so the time to the next one is
        // memoryless and the model is back in its state at time 0. The run ends at the end
        // of a cycle, once its own CI is narrow enough, so the runs are independent too.
        double t = 0.0, start = 0.0;     // now, and the start of the current cycle
        long jobs = 0;                   // jobs served in the current cycle
        bool done = false;
        while (!done)
        {
            e = net.next_event();                                     // globally earliest event
            t = e->get_time();
            bool from_station = e->get_info(EVENT_NODE).second == 1;  // read it before route() moves e
            net.route(e);                                             // move it to its next node
            if (from_station)
            {
                ++jobs;
                if (sta->queue_length() + sta->service_length() == 0)   // regeneration point: close the cycle
                {
                    throughput->update(jobs, t - start, 0);
                    jobs = 0;
                    start = t;
                    done = throughput->n_updates(0) >= min_cycles
                        && narrow(throughput->run_confidence_interval(alpha, 0), run_tolerance);
                }
            }
        }
        net.reset(t, {}, true);          // close the run at the regeneration point, keep its statistics
        throughput->reset(true);         // store the run's throughput
    }
    while (throughput->completed_runs(0) < min_runs
           || !narrow(throughput->confidence_interval(alpha, 0), tolerance));

    auto [thr_lo, thr_hi] = throughput->confidence_interval(alpha, 0);
    auto [soj_lo, soj_hi] = sojourn->confidence_interval(alpha, 0);
    cout << throughput->completed_runs(0) << " runs\n"
         << "Throughput   95% CI [" << thr_lo << ", " << thr_hi << "]  (theory 0.8)\n"
         << "Mean sojourn 95% CI [" << soj_lo << ", " << soj_hi << "]  (theory 5)\n";
}
```

Output with Apple clang (other standard libraries draw different samples):

```
29 runs
Throughput   95% CI [0.786936, 0.802772]  (theory 0.8)
Mean sojourn 95% CI [4.47841, 5.14085]  (theory 5)
```

Within a run, cycle i serves n<sub>i</sub> jobs in time τ<sub>i</sub>. The throughput estimate is r = Σn<sub>i</sub> / Στ<sub>i</sub>, and its half-width is t·s / (τ̄·√k). Here s² is the sample variance of n<sub>i</sub> − r·τ<sub>i</sub>, τ̄ the mean cycle length, k the number of cycles and t the Student-t quantile with k − 1 degrees of freedom. The variance includes the covariance of n<sub>i</sub> and τ<sub>i</sub>, which is large, since longer cycles serve more jobs. That is why a ratio needs its own observer rather than two `scalar`s. `throughput->reset(true)` stores r as the run's estimate, and `throughput->confidence_interval()` is the Student-t interval across those estimates. `narrow()` compares an interval's half-width, (hi − lo) / 2, with the tolerance times its midpoint, (hi + lo) / 2. `min_cycles` and `min_runs` keep the first, unreliable intervals from a handful of samples from stopping a run, or the simulation, too early.

The [example walkthrough](docs/examples/basic-network.md) explains each step.

## Using the library

Install the headers and the library system-wide (uses `sudo`; installs to `/usr/local` on macOS and `/usr` on Linux), then compile against it:

```sh
make install
g++ -std=c++23 myprogram.cpp -ldes -o myprogram
```

To use the library from the working tree instead, point the compiler and the runtime linker at it:

```sh
g++ -std=c++23 -I/path/to/des/src myprogram.cpp -L/path/to/des -ldes -Wl,-rpath,/path/to/des -o myprogram
```

On macOS, `libdes.dylib` records `/usr/local/lib` as its location. The program will load an installed copy from there, if one exists, until you redirect it:

```sh
install_name_tool -change /usr/local/lib/libdes.dylib @rpath/libdes.dylib myprogram
```

Common build targets:

| Command | Effect |
|---|---|
| `make` | Release build (`-O3`) |
| `make debug` | Clean debug build (`-O0 -g`) |
| `make CXXSTD=c++20` | Build with another C++ standard |
| `make test` | Build and run `test/test` (add `LINK_INSTALLED=0` for the working-tree library) |
| `make check` / `make validate` | Unit and regression tests / validation against queueing theory |
| `make check-sanitize` / `make coverage` / `make bench` | Tests under sanitizers / coverage report / benchmarks |
| `make install` / `make uninstall` | Install or remove the headers and library system-wide |
| `make clean` / `make clean-lib` | Remove object files / the compiled library |

[Getting Started](docs/getting-started.md) lists all the build options, and the Makefile also has profiling targets (`make profile`). [Testing](docs/testing.md) describes the test pipeline and how to add tests.

## Documentation

| Document | Contents |
|---|---|
| [Getting Started](docs/getting-started.md) | Building, compiling your program, a minimal example |
| [Architecture](docs/architecture.md) | Class hierarchy, design patterns, the simulation loop, replications |
| [API Reference](docs/api/README.md) | Every class, grouped by header |
| [Example: M/M/1](docs/examples/basic-network.md) | Annotated walkthrough of `test/test.cpp` |

Topics you may want to read next:

- [Writing a queue discipline](docs/api/queue.md#implementing-a-custom-policy): LIFO and priority examples
- [Stations](docs/api/station.md): multi-server, processor sharing, GPS and class-priority stations
- [Routing and blocking handlers](docs/api/network.md#custom-handlers)
- [Tags](docs/api/tags.md): adding your own event fields and measuring them

## Repository layout

```
src/     library sources; public headers are src/libdes_*.hpp
test/    test.cpp: M/M/1 and M/M/2 models checked against queueing theory
docs/    user guide and API reference
```

## Notes

- The library is not thread-safe. To run replications in parallel, use separate processes.

## Contact

Questions, bug reports and suggestions are welcome as [GitHub issues](https://github.com/paolocastagno/des/issues).
