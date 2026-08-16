# VM Domain Model

## Core Concepts
- **VmId**: A strongly typed identity wrapping a string. Generatable deterministically or via UUID in the future.
- **VmConfig**: The aggregate root for VM configuration (CPU, Memory, Storage, Network, Firmware, Display). Enforces basic validity.
- **VmState**: The lifecycle state of a VM (`Created`, `Starting`, `Running`, `Paused`, `Stopping`, `Stopped`, `Failed`).

## Ownership and Delegation
- `VmManager` acts as the application service orchestrating VM operations.
- `IVmRepository` abstracts VM persistence/registry (saving configs and tracking state).
- `IVirtualizationBackend` abstracts hypervisor actions (start, pause, shutdown, etc.).
- Errors are returned as `std::expected` using the `VmError` enum for meaningful operational failures (e.g., `InvalidLifecycleTransition`, `VmNotFound`).

## Dependency Direction
Domain models do not depend on backend concepts. The core Application coordinates standard VM lifecycle semantics, oblivious to underlying technologies like QEMU.
