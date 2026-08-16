# ForensicVM

ForensicVM is a full-featured virtual machine management and forensic virtualization platform.

## Overview

ForensicVM provides normal VM capabilities alongside a dedicated forensic control plane. It emphasizes evidence integrity, reproducible analysis environments, and strict isolation controls.

## Architecture

The project is layered into:
* UI / Management
* VM Management Layer
* Forensic Control Plane
* Virtualization Abstraction
* Backend Adapter (initially QEMU/KVM)

## Building

Requires a modern C++23 compiler and CMake.
