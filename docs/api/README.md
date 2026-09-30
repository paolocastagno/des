# API Reference

| Class / Group | Header | Description |
|---|---|---|
| [`des::event`](event.md) | `libdes_event.hpp` | Discrete simulation event |
| [`des::queue` / policies](queue.md) | `libdes_queue.hpp`, `libdes_policy.hpp`, `libdes_store.hpp`, `libdes_fifo.hpp`, `libdes_is.hpp`, `libdes_ps.hpp` | Queues, disciplines and job stores |
| [`des::node`](node.md) | `libdes_node.hpp` | Abstract node base class |
| [`des::station`](station.md) | `libdes_station.hpp` | Generic service station |
| [`des::source` / `des::sink`](source-sink.md) | `libdes_source.hpp`, `libdes_sink.hpp` | Event generators and absorbers |
| [`des::network`](network.md) | `libdes_network.hpp` | Event routing and global clock |
| [Random streams](random.md) | `libdes_random.hpp` | xoshiro256** generator split into disjoint streams |
| [Observers](observers.md) | `libdes_scalar.hpp`, `libdes_counter.hpp`, `libdes_sample.hpp`, `libdes_histogram.hpp`, `libdes_ratio.hpp` | Measurement collectors |
| [`des::message`](message.md) | `libdes_message.hpp` | Key-value observer payload |
| [Tags](tags.md) | `libdes_tag.hpp` | Tag registry and tag-indexed field storage |
| [Constants](constants.md) | `libdes_const.hpp` | Named keys and signal strings |
