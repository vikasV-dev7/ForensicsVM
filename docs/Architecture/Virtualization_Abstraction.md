# Virtualization Abstraction

## Backend Abstraction
The `IVirtualizationBackend` contract resides at the architectural boundary (`src/vm/contracts`). It represents platform-neutral virtualization operations, completely decoupling the VM application domain from QEMU/KVM logic.

## Rationale
Domain code must not depend on QEMU APIs, arguments, or execution contexts.
- Avoids leaking `exec()`, `QMP` sockets, or libvirt configuration details into standard VM lifecycle management.
- Guarantees the application remains testable (e.g. `InMemoryBackend`).

## Future Implementations
- `QemuKvmAdapter`: Will implement `IVirtualizationBackend` to spawn `qemu-system-x86_64` processes and attach to QMP.
- Could theoretically support Hyper-V or others without touching `VmManager` logic.
