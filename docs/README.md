# libdes Documentation

Welcome to the **libdes** documentation — a C++ library for building Discrete Event Simulations (DES).

## Contents

| Document | Description |
|---|---|
| [Getting Started](getting-started.md) | Installation, building, and a minimal working example |
| [Architecture](architecture.md) | Design overview, key patterns, and how components interact |
| [API Reference](api/README.md) | Full reference for all classes and functions |
| [Examples](examples/basic-network.md) | Annotated example: M/M/1 open queueing system |

## Quick Navigation

### Core Simulation Components
- [Event](api/event.md) — the fundamental unit of simulation
- [Node](api/node.md) — abstract base for all stations
- [Station](api/station.md) — generic service station with pluggable distributions
- [Source / Sink](api/source-sink.md) — event generators and absorbers
- [Network](api/network.md) — routes events between nodes

### Queue Subsystem
- [Queue & Policies](api/queue.md) — queue management and FIFO / Infinite-Server policies

### Measurement
- [Observers](api/observers.md) — `scalar`, `counter`, `vector`, `histogram`

### Utilities
- [Message](api/message.md) — key-value container used for observer notifications
- [Constants](api/constants.md) — all named constants, info keys, and signal names
