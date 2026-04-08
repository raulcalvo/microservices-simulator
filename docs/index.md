# MicroDummy Microservices Architecture

This Proof of Concept implements a distributed microservices system acting as a dummy environment to simulate complex message flows.

## Key Features
* **Multi-purpose Binary:** A single `micro_node` executable that changes behavior based on the `--role` parameter.
* **Structured Logging:** All events are logged in JSON format mapping the transaction's event flow.
* **Chaos Engineering:** In-built chaos toggles to intentionally corrupt and inject failure scenarios to validate system resilience and monitoring.
