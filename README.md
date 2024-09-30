# Discrete Event Simulation (DES) Framework

### What is DES?

The **Discrete Event Simulation (DES)** C++ library is a framework designed to simplify the creation and management of event-based simulators. The library abstracts many core simulation components—such as event handling, queue management, and data collection—allowing developers to focus on modeling the actual system rather than worrying about the underlying simulation mechanics.

### Key Features of DES:

1. **Event Management**:
   - DES efficiently handles **future event lists (FEL)**, which queue events scheduled to occur at future time points. These events drive the simulation forward, as the simulation engine processes them in time order.

2. **Queue Management**:
   - DES provides base implementations for:
     - **Single and Multiple Server Queues**: Simulating systems with a limited or multiple number of service units (servers).
     - **First-In, First-Out (FIFO) Policy**: A queue policy where the first entity to enter the queue is the first to be processed.
     - **Infinite Server (IS) Queues**: Queues with an unlimited number of servers, where all arrivals are immediately served without delay.
   - Users can easily define and implement their own queuing policies if the predefined ones don’t fit the simulation requirements.

3. **Modular Station Network**:
   - A simulator built using DES is modeled as a **network of independent stations**. Each station maintains its own future event list (FEL), making it responsible for handling its events autonomously. 
   - The **simulation engine** coordinates these stations by polling them for their next event and managing their interactions. This modularity allows for the design of highly flexible and complex simulation systems.

4. **Customizability**:
   - DES is designed with flexibility in mind, allowing users to create **custom queue policies, stations, and events**. This makes the library adaptable to a wide range of simulation scenarios.

### Components of the DES Library:

- **Event Handling**: Events are the basic unit of simulation and represent a change in the system’s state at a specific time. Each station processes its own events, and the simulator engine ensures these events occur in the correct order.
  
- **Queue Management**: Handling entities waiting for service, using either predefined or custom queueing rules. Multiple queue disciplines (like FIFO, infinite server) are provided out-of-the-box, with room for user-defined policies.

- **Simulator Engine**: Manages the progression of simulation time by polling each station for its next event, ensuring the simulation runs smoothly in a chronological manner.

- **Measurements and Metrics**: DES provides basic facilities for collecting metrics related to simulation performance, allowing users to analyze system behavior such as wait times, queue lengths, and service times.

### Use Cases for DES:

- **Manufacturing Systems**: Simulate production lines with multiple stations, queues, and service times.
- **Network Traffic Simulation**: Model data packets moving through network routers with queuing policies and service delays.
- **Healthcare Systems**: Simulate patient flow through hospital departments, with waiting times, services, and resources being processed.
- **Custom Simulation Models**: Due to its flexible nature, DES can be adapted for any event-driven system that needs simulation.

## Installation

To compile the project, use the following commands:

```sh
g++ -c prog.c
g++ -o prog prog.o -ldes -lstdc++
```

## Usage

1. **Include the necessary headers** in your source files.
2. **Create and configure events, queues, and network nodes** as needed.
3. **Run the simulation** and collect the results for analysis.

## Example

Here is a simple example of how to use the framework:
```
TBD
```

<!-- 
```cpp
#include "libdes.hpp"

int main() {
    // Create a source node
    des::source src;

    // Create a sink node
    des::sink snk;

    // Create a FIFO queue
    des::fifo queue;

    // Schedule events and run the simulation
    src.schedule_event();
    queue.enqueue_event();
    snk.process_event();

    return 0;
} -->
```

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests.

<!-- ## License

This project is licensed under the MIT License. See the LICENSE file for details. -->

## Contact

For any questions or suggestions, please open an issue or contact the repository owner.