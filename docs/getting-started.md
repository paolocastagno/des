# Getting Started

## Prerequisites

- A C++23-capable compiler by default, or a C++20-capable compiler when building with `CXXSTD=c++20`
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
| `make` | Release build (default) |
| `make release` | Release build (`-O3`, `-DNDEBUG`) |
| `make debug` | Clean rebuild in debug mode (`-O0`, `-g`, `-DDEBUG`) |
| `make optimized` | Alias for `make release` |
| `make BUILD=release` | Release build without the clean step |
| `make BUILD=debug` | Debug build without the clean step |
| `make CXXSTD=c++20` | Build with a different C++ standard |
| `make install` | Build and install the headers and library system-wide |
| `make uninstall` | Remove the system-wide installed headers and library |
| `make clean` | Remove object files |
| `make clean-lib` | Remove the compiled library |

## Compiling Your Program

If you installed the library system-wide via `make install`, the headers and the library are on the default search paths:

```sh
g++ -std=c++23 -c myprogram.cpp
g++ -o myprogram myprogram.o -ldes
```

To use the library from the build directory instead, add its headers to the include path, link against it, and record its directory for the runtime linker:

```sh
g++ -std=c++23 -I/path/to/des/src -c myprogram.cpp
g++ -o myprogram myprogram.o -L/path/to/des -ldes -Wl,-rpath,/path/to/des
```

On macOS one more step is needed. `libdes.dylib` records its install location, `/usr/local/lib/libdes.dylib`, and the program looks for the library there. It then either fails to start or silently loads a previously installed, possibly older, copy. Make it search the rpath instead:

```sh
install_name_tool -change /usr/local/lib/libdes.dylib @rpath/libdes.dylib myprogram
```

## Minimal Example

The snippet below creates a single-queue M/M/1 simulation with a source, one FIFO station, and a sink.

```cpp
#include <memory>
#include <random>
#include <vector>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_source.hpp>
#include <libdes_sink.hpp>
#include <libdes_station.hpp>
#include <libdes_network.hpp>

int main()
{
    // Arrival rate lambda = 0.8 events/time unit — exponential inter-arrival time
    auto arr  = std::make_shared<std::exponential_distribution<double>>(0.8);
    auto src  = std::make_shared<des::source<double, std::exponential_distribution>>(
                    std::vector<std::shared_ptr<std::exponential_distribution<double>>>{arr},
                    "Source");

    // Service rate mu = 1.0 — exponential service time
    auto svc  = std::make_shared<std::exponential_distribution<double>>(1.0);
    auto sta  = std::make_shared<des::station<double, std::exponential_distribution>>(
                    std::vector<std::vector<std::shared_ptr<std::exponential_distribution<double>>>>{{{svc}}},
                    1,      // number of servers
                    1,      // server capacity
                    1,      // number of waiting queues
                    100,    // queue capacity
                    "M/M/1");

    auto snk  = std::make_shared<des::sink>("Sink");

    // Routing matrix: source(0) -> station(1) -> sink(2)
    std::vector<std::vector<std::vector<double>>> routing = {
        {{0}, {1}, {0}},
        {{0}, {0}, {1}},
        {{0}, {0}, {0}}
    };

    // The seed (42) makes the run reproducible: every node gets its own random streams
    des::network net({src, sta, snk}, routing, 42);

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
        net.route(e);
    }

    return 0;
}
```

See [Examples](examples/basic-network.md) for a more complete walkthrough including measurement collection.

## Next Steps

- [Architecture](architecture.md) — understand how components fit together
- [API Reference](api/README.md) — full class and function reference
