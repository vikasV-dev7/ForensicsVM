# QEMU Machine Protocol (QMP) Architecture

## Overview
Phase 2C introduces a deterministic, local QMP control plane between ForensicVM and QEMU. QMP is treated strictly as an infrastructure implementation detail. The Domain layer (`VmManager`, `VmConfig`, `VmState`) remains completely unaware of QMP terminology and JSON processing.

## QMP Client Ownership
The `QemuBackend` coordinates the QMP lifecycle alongside process management. It maintains a 1:1 mapping between a `QemuProcess` and a `QmpClient`.
The `QmpClient` fully encapsulates the Windows Named Pipe client handle, JSON serialization (`nlohmann::json`), and QMP message parsing. The Backend layer translates QMP-specific outcomes into standardized `domain::VmError` returns.

## Windows Named Pipe Architecture
- QEMU is launched with `-chardev pipe,id=qmp0,path=fvm-qmp-<VmId> -mon chardev=qmp0,mode=control`.
- QEMU creates the named pipe as a server (`\\.\pipe\fvm-qmp-<VmId>`).
- ForensicVM acts as the Named Pipe Client, connecting via `CreateFileA`.
- This ensures strictly localized, secure communication without TCP exposure or Firewall interference.

## Message Classification & Event Buffering
Incoming JSON messages are deterministically classified:
- Contains `"return"` -> Command Response
- Contains `"error"` -> Command Error
- Contains `"event"` -> Asynchronous Event

The `QmpClient::execute()` method is synchronous. If asynchronous events are encountered while blocking for a command response, they are pushed into a `pendingEvents_` buffer, which can be drained later via `pollEvents()`.

## Concurrency Model
Only **one** QMP execute transaction can be active per connection at any time. `execute()` is protected by a standard `std::mutex`. No dedicated reader threads or asynchronous request-ID multiplexing are implemented.

## Lifecycle Mapping
| Domain Operation | QMP Command |
| --- | --- |
| `pauseVm()` | `stop` |
| `resumeVm()` | `cont` |
| `shutdownVm()` | `system_powerdown` |
| `powerOffVm()` | `quit` |
| `queryState()` | `query-status` |

## State Authority
When QMP is successfully connected, `query-status` is authoritative for the VM state. If QMP drops but the `QemuProcess` remains alive, the Backend reports an infrastructure failure (`BackendUnavailable`) to avoid falsely projecting a nominal Domain state.

## Security
QMP inherits ForensicVM's execution security context via the Windows Named Pipe's default Security Descriptor. No remote access, guest networking, USB passthrough, or arbitrary host command execution features are enabled.

## Phase 2C Non-Goals
This architecture currently excludes remote QMP multiplexing, dynamic snapshot management, hotplug events, memory/disk acquisition, and guest OS interaction.
