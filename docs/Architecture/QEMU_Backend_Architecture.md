# QEMU Backend Architecture (Phase 2B)

## Overview
The `QemuBackend` implements the `IVirtualizationBackend` boundary. It abstracts the platform-specific execution of QEMU on Windows and ensures that the core domain model remains entirely ignorant of QEMU-specific constructs, executable paths, or hypervisor features.

## Architecture

```text
VmManager
    |
    v
IVirtualizationBackend
    |
    v
QemuBackend
    |
    ├── QemuLocator (Discovers and validates QEMU executable)
    │
    ├── QemuCommandBuilder (Translates VmConfig -> QemuLaunchSpec)
    │
    └── QemuProcess (Abstracts OS-level child process management)
```

### Components

#### 1. QemuLocator
Responsible for discovering the `qemu-system-x86_64` executable either via explicit configuration or by probing the system `PATH`. Crucially, it validates the binary by invoking it with `--version` via direct process creation (bypassing any shell to prevent shell injection) and checking its output to guarantee we are not launching a counterfeit or incompatible binary.

#### 2. QemuCommandBuilder
Translates a platform-neutral `domain::VmConfig` into a `QemuLaunchSpec`. To prevent injection vulnerabilities, the `QemuLaunchSpec` represents arguments as an explicit array (`std::vector<std::string>`) rather than a single formatted command string.
*Safe Defaults:* The builder automatically appends `-nodefaults` and refuses to expose host folders, clipboard, networking, or USB devices by default.

#### 3. QemuProcess
Abstracts Windows process management (`CreateProcessA`, `TerminateProcess`). It launches QEMU silently without console windows attached to ForensicVM. 
By encapsulating OS-level process management here, we keep platform-specific APIs out of the `QemuBackend` core logic.
* **Windows Command-Line Quoting:** The boundary between the argument vector and `CreateProcessA` implements strict, safe Windows command-line quoting to prevent argument injection via spaces or quotes.
* **Job Object Containment:** The QEMU child process is immediately assigned to a Windows Job Object configured with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`. This guarantees that if ForensicVM unexpectedly crashes or terminates, the OS automatically tears down the QEMU process, leaving no orphaned VMs.

#### 4. QemuBackend
Coordinates the subordinate components. In Phase 2B, `startVm` relies on an in-memory configuration cache populated during `createVm`, maintaining strict compatibility with the `IVirtualizationBackend` contract that operates strictly on `VmId` for lifecycle operations.

## Security & Isolation
- QEMU remains an external process and is not statically linked into ForensicVM.
- No network, host folders, or USB passthrough are configured by default.
- Process runs with standard privileges.

## Deferred Features (Phase 2C+)
- **QMP (QEMU Machine Protocol):** Interaction currently relies entirely on OS-level signals (forced termination). Graceful shutdown, real-time metrics, and pause/resume capabilities require QMP. `shutdownVm` currently behaves identically to `powerOffVm` (forced shutdown) as a Phase 2B limitation.
- **Guest Boot:** Currently restricted to headless testing states without functional OS disks.
