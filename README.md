### Felix OS

## **Felix** (Fast Embedded Lightweight Interactive eXecutive) is a minimalist, cooperative, POSIX-style operating system.

The goal of Felix is to provide a "Linux-feeling" development environment for embedded engineers without the overhead, complexity, or non-deterministic nature of a preemptive kernel.

---

## 🚀 Hardware & Portability Roadmap

Because Felix OS strictly isolates its Hardware Abstraction Layer (HAL) from the Kernel and POSIX layers, porting to new SoCs simply requires swapping out the UART driver and interrupt macros. Approximately 80% of the OS (VFS, Shell, POSIX layer) is entirely hardware-agnostic.

✅ RISC-V (64-bit): Microchip PolarFire SoC (Current Lead Target)

⏳ ARM Cortex-M4 (32-bit): NXP LPC54S018M-EVK (Planned)

⏳ ARM Cortex-M55 (32-bit): ALIF Ensemble Family (Planned)

---

## 🛠️ Adding Custom Commands (The argc, argv approach)

One of Felix's most powerful features is how easily you can add new commands. You do not need to write complex kernel modules or learn a proprietary API.

Felix treats internal commands exactly like standard C executables. You just write a function using the standard POSIX signature (int my_cmd(int argc, char **argv)) and register it to the VFS using a single macro: FLX_VFS_DECLARE_CMD.

Example: Adding a custom hello command:

```
/* 1. Write standard C code */
int cmd_hello(int argc, char **argv) {
    if (argc > 1) {
        printf("Hello, %s!\n", argv[1]);
    } else {
        printf("Hello, World!\n");
    }
    return 0;
}

/* 2. Mount it as a file in the VFS! */
FLX_VFS_DECLARE_CMD(node_hello, "hello", &node_bin, cmd_hello);
```

Once mounted, the shell's standard execvp layer automatically finds it in /bin/hello, parses the user's quotes and variables via wordexp, and passes the resulting argc and argv directly to your function!

---

## 🚀 Purpose
Modern embedded systems often sit in an awkward gap: they are too powerful for simple "while(1)" super-loops, but full-blown Linux is often too heavy, slow to boot, or difficult to interface with specialized FPGA fabric. 

**Felix** fills this gap by providing:
* **Familiarity:** Standard POSIX calls (`open`, `read`, `write`, `printf`) so Unix engineers feel at home immediately.
* **Control:** (Planned) A cooperative scheduler that ensures tasks only yield when they are ready, simplifying state management and reducing context-switch overhead.
* **Performance:** Designed to boot in milliseconds and run directly in high-speed SRAM (LIM/L2) for maximum throughput.

---

## 🎯 Primary Goals
1.  **POSIX-Compatible Stubs:** Replace standard `Newlib` stubs to map `stdio` and system calls to internal Felix services and hardware drivers.
2.  **Cooperative Streaming Architecture:** (Planned) A microkernel design focused on piping data blocks between tasks and hardware interfaces without blocking.
3.  **Static VFS:** A virtual filesystem mapping `/dev/` and `/bin/` to static function trees and device drivers.
4.  **Interactive Shell:** The **`felix`** CLI, providing a robust, command-line interface for system debugging and control.
5.  **Multi-Platform Portability:** **(In the works)** A clean separation between the kernel logic and the Hardware Abstraction Layer (HAL) for RISC-V and ARM (planned) architectures.

---

## 🛠 Target Hardware
* **Microchip PolarFire SoC:** Focusing on the U54 application harts.
* **NXP LPC54S018M** **(Planned)** First ARM.
* **Alif Semiconductor:** **(Planned)** Targeting both the High-Performance and High-Efficiency cores.

---

## ⚖️ The Cooperative Philosophy **(Planned)**
Unlike preemptive RTOSes, Felix operates on a **"Polite Citizen"** model. 
* **No Preemption:** Tasks are never forcibly interrupted by the scheduler.
* **Explicit Yielding:**  Performance and responsiveness are managed by the programmer using `yield()`, ensuring that context switches only happen at logical boundaries.
* **Deterministic I/O:**  Ideal for streaming large data sets where timing and sequence are critical.

---

## 📂 Project Structure 
* `/arch`: **(Planned)** Architecture-specific assembly and context-switching (RISC-V/ARM).
* `/kernel`: The core scheduler and task management.
* `/posix`: Syscall implementations and standard C library "un-stubbing."
* `/vfs`: Virtual File System and device registration logic.
* `/shell`: The `frell` CLI and userland utilities.
* `/platform`: Hardware-specific drivers (HAL).

---

## 📂 VFS Structure 
Add as many directories, subdirectories, files and executables as your project needs.
* `/`: The root directory.
* `/bin`: A bin directory.
* `/dev`: (in process) A dynamic device directory.
* `/test`: A home for Unit tests.
