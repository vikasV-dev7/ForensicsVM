# ADR 0001: Initial Architecture

## Status
Accepted

## Context
We are establishing the engineering foundation for ForensicVM, a platform merging VM management with forensic isolation.

## Decisions

1. **Language & Standard**: Use Modern C++23. It provides strong type safety, RAII, and performance needed for systems programming.
2. **Build System**: Use CMake. It is the industry standard for C++, enabling cross-platform builds and easy integration with testing frameworks.
3. **Virtualization Abstraction**: Create an abstraction layer over virtualization backends. Domain-level code must not directly depend on QEMU/KVM APIs.
4. **Initial Target**: QEMU/KVM will be our first backend adapter. Writing a custom hypervisor is out of scope.
5. **Separation of Concerns**: Separate regular VM management from the forensic control plane. The forensic layer must have absolute authority over evidence protection.
6. **Evidence Preservation**: All evidence files will be mounted read-only, using copy-on-write overlays for execution/analysis.
7. **Incremental Delivery**: Build incrementally, ensuring each subsystem is fully tested before integrating the next.

## Consequences
- Requires strict interface segregation to keep QEMU-specific code isolated.
- The forensic control plane will act as a strict gatekeeper for I/O operations involving evidence drives.
