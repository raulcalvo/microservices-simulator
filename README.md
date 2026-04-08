# MicroDummy

MicroDummy is a lightweight C++20 Proof of Concept (PoC) that simulates a generic distributed microservices system. It is designed to emulate various common network architectures and message flows (such as scatter-gather, batching, and conditional routing) to help you validate your monitoring infrastructure, test resilience, and observe structural data flows.

## Features

- **Single Multi-Purpose Binary**: A single `micro_node` executable that changes behavior intelligently based on the `--role` parameter.
- **Seven Core Topologies**: Built-in simulations for Happy Paths, Batching, Latency, Orphan Timeouts, Conditional Routing, Scatter-Gather, and Data Corruption.
- **Chaos Engineering**: Built-in `--chaos` flags to intentionally drop packets, corrupt JSON schemas, and simulate P95 latency spikes.
- **Structured Logging**: All telemetry and inter-node events are logged exclusively in pure JSON with strict correlation IDs mapping the distributed sequence.

## Architecture

The system utilizes ZeroMQ (`cppzmq`) for ultra-fast inter-process communication and `spdlog` / `nlohmann-json` for strict structured JSON logging. Documentation is built with MkDocs.

## Quick Start (Docker)

The fastest way to witness the distributed mesh in action is via Docker Compose, which spins up all 7 flows concurrently.

```bash
docker compose up --build -d
```

View the generated telemetry events across the entire cluster:
```bash
docker compose logs -f
```

## Documentation

Full architectural sequence diagrams and payload schemas are located in the `docs/` folder.

To build and serve the docs locally using MkDocs:
```bash
pip install mkdocs-material mkdocs-mermaid2-plugin
mkdocs serve
```

## Building Locally 

If you wish to compile the C++ source natively without Docker:

### Prerequisites
- CMake 3.20+
- A C++20 compatible compiler (GCC 11+, Clang 14+, or MSVC)
- System package manager (e.g., `apt`) for dependencies: `libzmq3-dev`, `libspdlog-dev`, `nlohmann-json3-dev`, `libcxxopts-dev`

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
