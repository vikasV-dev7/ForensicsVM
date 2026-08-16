# Virtualization Threat Model (Preliminary)

This document outlines the preliminary security boundaries and threat model for ForensicVM prior to implementing the virtualization backend.

## Security Context
ForensicVM is intended for forensic investigation, which involves handling potentially malicious guest workloads (e.g., malware sandboxing, executing untrusted binaries) and guaranteeing the integrity of digital evidence.

## Critical Security Boundaries

1. **Host/Guest Isolation**
   - **Risk:** Guest escapes (VM escape vulnerabilities) via emulated hardware devices, shared folders, or the hypervisor itself.
   - **Mitigation Strategy:** Minimize exposed attack surface. Restrict device passthrough, disable unnecessary emulated hardware, and run QEMU with least privilege rather than as an elevated administrative user.

2. **Network Isolation**
   - **Risk:** Malware traversing the network to infect the host machine or exfiltrate data.
   - **Mitigation Strategy:** Implement strict "Network Quarantine" modes using Isolated/HostOnly adapters without NAT access to the WAN, or totally disconnected modes.

3. **Evidence Integrity**
   - **Risk:** Accidental or malicious mutation of the original forensic image disk.
   - **Mitigation Strategy:** All evidence drives must be mounted strictly read-only (`-drive file=...,readonly=on`). Any execution or analysis must occur via a copy-on-write overlay (e.g., qcow2 backing files). 

4. **Shared Folders and Clipboard**
   - **Risk:** Cross-contamination between host and guest.
   - **Mitigation Strategy:** Disabled by default. If required, must be explicitly enabled per session and ideally operate in a restricted/read-only mode.

5. **USB Passthrough**
   - **Risk:** Malicious firmware or drivers exploiting the host USB stack.
   - **Mitigation Strategy:** Restrict USB passthrough capabilities.

## Execution Privileges
The `VmManager` and `QemuBackend` should avoid running QEMU as Administrator on Windows or root on Linux. The subprocess should be launched with standard user privileges.
