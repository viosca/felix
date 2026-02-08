# Felix OS

**Felix** (Fast Embedded Lightweight Interactive eXecutive) is a minimalist, cooperative, POSIX-style operating system designed for modern SoCs like the **Microchip PolarFire SoC** and **Alif Semiconductor Ensemble/Fusion** boards.

The goal of Felix is to provide a "Linux-feeling" development environment for embedded engineers without the overhead, complexity, or non-deterministic nature of a preemptive kernel.

---

## 🚀 Purpose
Modern embedded systems often sit in an awkward gap: they are too powerful for simple "while(1)" super-loops, but full-blown Linux is often too heavy, slow to boot, or difficult to interface with specialized FPGA fabric. 

**Felix** fills this gap by providing:
* **Familiarity:** Standard POSIX calls (`open`, `read`, `write`, `printf`) so Unix engineers feel at home immediately.
* **Control:** A cooperative scheduler that ensures tasks only yield when they are ready, simplifying state management and reducing context-switch overhead.
* **Performance:** Designed to boot in milliseconds and run directly in high-speed SRAM (LIM/L2) for maximum throughput.

---

## 🎯 Primary Goals
1.  **POSIX-Compatible Stubs:** Replace standard `Newlib` stubs to map `stdio` and system calls to internal Felix services and hardware drivers.
2.  **Cooperative Streaming Architecture:** A microkernel design focused on piping data blocks between tasks and hardware interfaces without blocking.
3.  **Static VFS:** A virtual filesystem mapping `/dev/`, `/bin/`, and `/etc/` to static function trees and device drivers.
4.  **Interactive Shell:** The **`frell`** CLI, providing a robust, command-line interface for system debugging and control.
5.  **Multi-Platform Portability:** A clean separation between the kernel logic and the Hardware Abstraction Layer (HAL) for RISC-V and ARM architectures.

---

## 🛠 Target Hardware
* **Microchip PolarFire SoC:** Focusing on the U54 application harts.
* **Alif Semiconductor:** Targeting both the High-Performance and High-Efficiency cores.

---

## ⚖️ The Cooperative Philosophy
Unlike preemptive RTOSs, Felix operates on a **"Polite Citizen"** model. 
* **No Preemption:** Tasks are never forcibly interrupted by the scheduler.
* **Explicit Yielding:** Performance and responsiveness are managed by the programmer using `yield()`, ensuring that context switches only happen at logical boundaries.
* **Deterministic I/O:** Ideal for streaming large data sets where timing and sequence are critical.

---

## 📂 Project Structure (Planned)
* `/arch`: Architecture-specific assembly and context-switching (RISC-V/ARM).
* `/kernel`: The core scheduler and task management.
* `/posix`: Syscall implementations and standard C library "un-stubbing."
* `/vfs`: Virtual File System and device registration logic.
* `/shell`: The `frell` CLI and userland utilities.
* `/platform`: Hardware-specific drivers (HAL).

---

> *"Felix: Because embedded software doesn't have to be a mess."*
