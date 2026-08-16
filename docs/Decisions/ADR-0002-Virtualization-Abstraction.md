# ADR 0002: Virtualization Abstraction

## Status
Accepted

## Context
As ForensicVM progresses, we must protect the domain and application layers from becoming entangled with specific hypervisor technologies (e.g. QEMU, KVM, libvirt). 

## Decisions
1. **Separate Virtualization Contract**: Create an `IVirtualizationBackend` interface representing neutral VM execution capabilities (create, start, stop, pause, destroy, query).
2. **Infrastructure Layer**: Concrete implementations (like `QemuAdapter` later, or `InMemoryBackend` now) belong exclusively in `src/infrastructure/`.
3. **VM Repository**: Create an `IVmRepository` to handle registration and persistence independently of execution.
4. **No QEMU Concepts in Domain**: The application domain will not expose QEMU-specific flags, devices, or APIs.

## Consequences
- The architecture is strictly decoupled and unit-testable using in-memory stubs.
- Building the QEMU adapter will require careful mapping of domain configs (e.g., `StorageConfig`) to backend arguments.
