# Testing

The test pipeline has four tiers, driven by Makefile targets and run by GitHub Actions on every push. Every target compiles the workspace sources (`src/`) together with the tests into its own directory under `build/`; the installed library is never used.

| Target | What it runs | Time (M-series Mac) |
|---|---|---|
| `make check` | unit and regression tests | < 1 s (+ first build) |
| `make validate` | validation against queueing theory | ~2 s |
| `make check-sanitize` | unit and regression tests under AddressSanitizer + UndefinedBehaviorSanitizer, thread tests under ThreadSanitizer | ~15 s |
| `make coverage` | all tiers instrumented; line/branch coverage of `src/` in `build/coverage/` (clang) | ~10 s |
| `make bench` | benchmark table (events per second), not pass/fail | ~3 s |
| `make golden` | writes the regression golden values for this platform | < 1 s |
| `make ci` | `check`, `validate` and `check-sanitize` | |
| `make clean-build` | removes `build/` | |

Add `-j` to build in parallel (`make -j10 check`). `TEST_ARGS` passes options to the runner, e.g. `make check TEST_ARGS=-v` prints the notes of passing tests.

---

## Layout

```
test/
  harness/     des_test.hpp (macros), main.cpp (runner)
  support/     des_models.hpp: model builders shared by the suites and the benchmarks
  unit/        one file per component: tags, events/messages, stores/policies, node, network, observers, event pool
  regression/  test_golden.cpp and golden/<platform>/<model>.txt
  validation/  test_theory.cpp
  bench/       bench.cpp
  test.cpp     the M/M/1 and M/M/2 demo run by `make test`
```

## Tiers

**Unit** (`[unit]`): deterministic checks of one component at a time, with fixed seeds or deterministic service times (`des_models::fixed()`), e.g. exact processor-sharing departure times, the order of the job stores, the multi-class refill of a node, blocking and loss counters of the network. Tests involving threads also carry `[threads]` and run under ThreadSanitizer.

**Regression** (`[regression]`): fixed-seed models (M/M/1, M/M/2, tandem, weighted PS, infinite server, loss station, closed network) whose per-run results are compared with golden values, relative tolerance 1e-9. Any change in behaviour, intended or not, shows up here.

`<random>` distributions are implementation-defined and compilers may fuse floating-point operations differently, so golden values are kept per platform: `test/regression/golden/<stdlib>-<compiler>-<arch>/`, e.g. `libcxx-clang-arm64`. On a platform without golden values the regression tests are reported as skipped. After an **intended** behaviour change, or for a new platform:

```bash
make golden
```

then review `git diff test/regression/golden` before committing. The CI uploads the values generated on each runner as an artifact (`golden-<os>-<compiler>`), to commit the Linux ones.

**Validation** (`[validation]`): simulated estimates against queueing theory: M/M/1, M/M/2 (Erlang-C), M/D/1 (Pollaczek-Khinchine), M/M/1-PS and M/D/1-PS (insensitivity), multi-class PS, weighted PS (work conservation), M/M/∞, M/M/2/2 (Erlang-B), a Jackson tandem and a closed network (mean value analysis). One more M/M/1 check uses the regenerative method: a single run from an empty system, with no warm-up, where `des::ratio` estimates throughput and mean sojourn time from 20 000 regeneration cycles. Each of the other models runs 10 replications after a warm-up, and passes when the theoretical value lies in the replications' 99.9% Student-t confidence interval. Seeds are fixed, so the verdict is the same on every run of a platform; the 99.9% level keeps the chance of a false failure on a new platform small.

**Sanitizers**: the unit and regression tiers under ASan + UBSan (memory errors, undefined behaviour), and the `[threads]` tests under TSan (data races between replications running on parallel threads).

## Running a subset

The runner accepts filters: `[tag]` includes a tag, `~[tag]` excludes one, any other text selects the tests whose name contains it.

```bash
make build/check/des_tests
build/check/des_tests --list "[network]"
build/check/des_tests -v "[validation]" ~[threads]
build/check/des_tests "processor"
```

## Writing a test

Add a `.cpp` file (or a test case to an existing one) under `test/unit`, `test/regression` or `test/validation`; the Makefile picks it up.

```cpp
#include "des_models.hpp"
#include "des_test.hpp"

using namespace des_models;

TEST_CASE("a station serves in arrival order", "[unit][node]")
{
    det_station sta(det_dists{{fixed(10)}}, 1, 1, 1, INT_MAX, "Q");
    arrive(sta, 0, 0.0);
    auto second = arrive(sta, 0, 1.0);
    REQUIRE(second != nullptr);                     // stops the test case on failure
    sta.departure();
    CHECK_EQ(second -> get_info(NODE_SERVICE_START).second, 10.0);
    NOTE("printed on failure, or with -v");
}
```

| Macro | |
|---|---|
| `TEST_CASE(name, tags)` | defines a test case |
| `CHECK(cond)` / `REQUIRE(cond)` | records a failure / also stops the test case |
| `CHECK_EQ(a, b)` / `CHECK_NEAR(a, b, tol)` | compares, printing both values on failure |
| `CHECK_IN(value, interval)` | `interval.first <= value <= interval.second` |
| `CHECK_THROWS_AS(type, stmt)` / `CHECK_NOTHROW(stmt)` | exception checks (the statement may contain commas) |
| `NOTE(stream)` | context shown if the test fails, or with `-v` |
| `SKIP(stream)` | stops the test case and reports it as skipped |

Use a fixed seed for every network (`des::network(nodes, routing, seed)`, or the `seed` of `des_models::open_model`), so that a failure can be reproduced. Validation checks should compare against a confidence interval at a high level (`0.001`) rather than a fixed tolerance.

## Continuous integration

`.github/workflows/ci.yml` runs on every push and pull request, and nightly:

| Job | Platforms | Runs |
|---|---|---|
| check + validate | Linux (gcc, clang), macOS (clang) | `make check`, `make validate`; uploads the platform's golden values |
| sanitizers | Linux, macOS (clang) | `make check-sanitize` |
| coverage | Linux (clang) | `make coverage`; summary in the job page, HTML report as artifact |
| benchmarks | macOS, not on pull requests | `make bench`, table in the job page; never fails the workflow |
