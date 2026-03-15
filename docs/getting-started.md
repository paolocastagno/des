# Getting Started

## Prerequisites

- A C++20-capable compiler (`g++` >= 10 or `clang++` >= 12)
- GNU Make
- macOS or Linux

## Building the Library

Clone the repository and build with Make:

```sh
git clone <repository-url>
cd des
make
```

This produces a shared library file:
- `libdes.dylib` on macOS
- `libdes.so` on Linux

### Build Options

| Command | Effect |
|---|---|
| `make` | Debug build (default) |
| `make DEBUG=0` | Optimised build (no `-g`, with `-O3`) |
| `make install` | Build and run `install.sh` to install system-wide |
| `make clean` | Remove object files |
| `make clean-lib` | Remove the compiled library |

## Compiling Your Program

Add the path to the library headers to your include path and link against `libdes`:

```sh
g++ -std=c++20 -I/path/to/des/src -c myprogram.cpp
g++ -o myprogram myprogram.o -L/path/to/des -ldes
```

If you installed the library system-wide via `make install`:

```sh
g++ -std=c++20 -c myprogram.cpp
g++ -o myprogram myprogram.o -ldes
```

## Minimal Example

The snippet below creates a single-queue M/M/1 simulation with a source, one FIFO station, and a sink.

```cpp
#include <memory>
#include <random>
#include <exponential_distribution>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_source.hpp>
#include <libdes_sink.hpp>
#include <libdes_station.hpp>
#include <libdes_network.hpp>

int main()
{
    // Shared random-number generator (seeded for reproducibility)
    auto gen = std::make_shared<std::mt19937_64>(42);

    // Arrival rate lambda = 0.8 events/time unit
    auto src  = std::make_shared<des::source>(std::vector<double>{0.8}, "Source", gen);

    // Service rate mu = 1.0 — exponential service time
    auto svc  = std::make_shared<exponential_distribution<double>>(1.0);
    auto sta  = std::make_shared<des::station<double, exponential_distribution>>(
                    std::vector<std::vector<std::shared_ptr<exponential_distribution<double>>>>{{{svc}}},
                    1,      // number of servers
                    1,      // server capacity
                    1,      // number of event classes
                    100,    // queue capacity
                    "M/M/1",
                    gen);

    auto snk  = std::make_shared<des::sink>("Sink");

    // Routing matrix: source(0) -> station(1) -> sink(2)
    std::vector<std::vector<std::vector<double>>> routing = {
        {{0}, {1}, {0}},
        {{0}, {0}, {1}},
        {{0}, {0}, {0}}
    };

    des::network net({src, sta, snk}, routing, gen);

    // Inject the first event
    auto e = std::make_shared<des::event>();
    e->set_cls(0);
    e->set_time(0.0);
    e->set_info(EVENT_NODE, 0);
    src->arrival(e);

    // Run 50 000 events
    for (int i = 0; i < 50000; ++i)
    {
        e = net.next_event();
        net.route(e, e->get_time());
    }

    return 0;
}
```

See [Examples](examples/basic-network.md) for a more complete walkthrough including measurement collection.

## Next Steps

- [Architecture](architecture.md) — understand how components fit together
- [API Reference](api/README.md) — full class and function reference
