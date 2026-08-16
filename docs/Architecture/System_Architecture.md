# System Architecture

## Project Vision
ForensicVM is a virtual machine platform that merges modern desktop virtualization with a dedicated, isolated forensic control plane. It ensures evidence integrity, supports reproducible investigation environments, and provides deep forensic insights through snapshotting, memory acquisition, and behavioral monitoring.

## Architectural Layers

1. **UI / Management**
   - Handles user interaction, CLI commands, API requests.
2. **VM Management Layer**
   - Coordinates VM lifecycles (creation, execution, snapshotting) and resource allocation.
3. **Forensic Control Plane**
   - Enforces security boundaries, manages cryptographic evidence integrity, and ensures read-only evidence mounting with copy-on-write analysis layers.
4. **Virtualization Abstraction**
   - Defines a unified API for hypervisor operations, shielding the core system from backend-specific logic.
5. **Backend Adapter**
   - Translates virtualization abstraction commands to specific hypervisors.
6. **Hypervisor Backend (Initially QEMU/KVM)**
   - Executes the actual virtualization.

## Responsibilities & Boundaries

- **Forensic Control Plane** ensures that evidence is NEVER mutated. It handles cryptographic hashing and maintains a chain of custody log.
- **Dependency Direction** must flow inwards. The core logic depends on interfaces defined in the Virtualization Abstraction layer, not on specific backend implementations.

## Design Decisions
- **QEMU/KVM as Initial Backend**: Reimplementing a hypervisor from scratch is outside the project's scope. QEMU/KVM provides robust virtualization, hardware emulation, and strong community support.
- **No Custom Hypervisor**: Focus remains on the forensic features and orchestration, leveraging existing, proven hypervisors.
- **Security Boundaries**: The platform assumes a hostile guest environment. Isolation Lockdown and Evidence Lock modes prevent network exfiltration and host contamination.
