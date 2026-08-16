# Host Virtualization Environment

## Detected Environment
- **Operating System:** Windows 11 Home (64-bit)
- **CPU Virtualization:** Supported (Virtualization-based security is running, Base Virtualization Support is present).
- **Hypervisor Capabilities:** Windows 11 Home supports the Windows Hypervisor Platform (WHPX) and Virtual Machine Platform (VMP), though it lacks the full Hyper-V Management toolset.

## QEMU Availability
- **Status:** Completely Absent.
- No binaries (`qemu-system-x86_64`, `qemu-img`) were found in the system PATH or common installation directories.

## Recommended Backend Strategy
**Option A: QEMU + WHPX on Windows**
Given the host is Windows 11 Home, full native Hyper-V is unavailable, but WHPX (Windows Hypervisor Platform) is supported. QEMU for Windows can utilize WHPX to achieve near-native hardware acceleration without requiring a Linux/WSL boundary. This avoids the severe performance penalties of running QEMU without hardware acceleration (TCG mode).

*Tradeoffs:* WHPX on QEMU is highly performant but can sometimes lag behind KVM in specific experimental features. However, for a cross-platform C++ application running directly on Windows, this is the most native and efficient path forward.

## QEMU Integration Strategy
The ForensicVM application should integrate with QEMU as a **Managed Child Process**.
- **Process Management:** The `QemuBackend` adapter will launch `qemu-system-x86_64.exe` as a subprocess.
- **Control:** The adapter will communicate with QEMU via **QMP (QEMU Machine Protocol)** over a local socket to perform lifecycle operations (start, pause, shutdown, query state) reliably.
- **Dependency Policy:** ForensicVM will NOT link against QEMU libraries or libvirt natively. The boundary is strictly IPC via process invocation and QMP.

## Unresolved Environment Requirements
To proceed with Phase 2, the following must be installed:
1. **QEMU for Windows** (including `qemu-system-x86_64` and `qemu-img`).
2. **WHPX** feature must be verified as enabled via Windows Optional Features.

## QEMU Installation Requirements

**Recommended QEMU Distribution:**
The official QEMU binaries compiled for Windows via Stefan Weil's trusted standalone installer (https://qemu.weilnetz.de/w64/). While MSYS2 packages are available, the standalone installer is recommended for a stable, system-wide predictable path (`C:\Program Files\qemu`). 

**Required Binaries:**
- `qemu-system-x86_64.exe`: The core hypervisor binary used to emulate the x86-64 guest hardware.
- `qemu-img.exe`: The disk image utility used to create, convert, and manage forensic disk images (specifically creating `qcow2` copy-on-write overlays over read-only evidence).

**WHPX Requirements:**
The "Windows Hypervisor Platform" feature MUST be explicitly enabled to allow QEMU to use hardware virtualization (`-accel whpx`). It must be operational, not just available.
Verification command (requires elevated PowerShell):
`Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform`
It must report `State : Enabled`.

**Installation Procedure (Manual):**
1. Download the latest 64-bit installer from https://qemu.weilnetz.de/w64/ (e.g., `qemu-w64-setup-*.exe`).
2. Run the installer and install to the default directory (`C:\Program Files\qemu`).
3. Add `C:\Program Files\qemu` to your System or User `PATH` environment variable.
4. Open a new terminal.
5. Verify installation: `qemu-system-x86_64 --version`
6. Verify disk utility: `qemu-img --version`
7. Verify WHPX: Ensure Windows Optional Feature is enabled via the command above.

**Safe Minimal Test:**
Run this command to test QEMU and WHPX without requiring a guest OS disk:
`qemu-system-x86_64 -accel whpx -m 512 -display sdl -nodefaults -vga std`
*(Success criteria: A QEMU display window appears showing a "No bootable device" or BIOS screen, no fatal WHPX errors are printed in the terminal, and closing the window successfully exits the process.)*

**Security Considerations & Baseline:**
- QEMU should run with standard user privileges, never as Administrator.
- No shared folders, clipboard sharing, host filesystem exposure, USB passthrough, or active network connection by default for forensic VMs.
- Evidence disks MUST be attached as `readonly=on`.
- Analysis writes will strictly be routed to disposable `qcow2` backing layers.
- VM state is auditable and destructive actions require explicit user intent.

**External Dependency Policy:**
QEMU binaries are an external runtime dependency. They MUST NOT be copied into the Git repository. Do not commit QEMU executables, DLLs, VM disk images, or ISOs. The `VmManager` will eventually be configured with the dynamic path to the host's QEMU installation.
