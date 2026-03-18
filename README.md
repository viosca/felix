## **Felix OS**

## **Felix** (Fast Embedded Lightweight Interactive eXecutive) is a minimalist, cooperative, POSIX-style operating system.
The goal of Felix is to provide a "Linux-feeling" development environment for embedded engineers without the overhead, complexity, or non-deterministic nature of a preemptive kernel.

## 🚀 Hardware & Portability Roadmap
Because Felix OS strictly isolates its Hardware Abstraction Layer (HAL) from the Kernel and POSIX layers, porting to new SoCs simply requires swapping out the UART driver and interrupt macros. Approximately 80% of the OS (VFS, Shell, POSIX layer) is entirely hardware-agnostic.

✅ RISC-V (64-bit): Microchip PolarFire SoC (Current Lead Target)

⏳ ARM Cortex-M4 (32-bit): NXP LPC54S018M-EVK (Planned)

⏳ ARM Cortex-M55 (32-bit): ALIF Ensemble Family (Planned)


## 🚀 Purpose
Modern embedded systems often sit in an awkward gap: they are too powerful for simple "while(1)" super-loops, but full-blown Linux is often too heavy, slow to boot, or difficult to interface with specialized FPGA fabric.

**Felix** fills this gap by providing:
* **Familiarity:** Standard POSIX calls (`open`, `read`, `write`, `printf`) so Unix engineers feel at home immediately.
* **Control:** (Planned) A cooperative scheduler that ensures tasks only yield when they are ready, simplifying state management and reducing context-switch overhead.
* **Performance:** Designed to boot in milliseconds and run directly in high-speed SRAM (LIM/L2) for maximum throughput.

## 🎯 Primary Goals

* **POSIX-Compatible Stubs:** Replace standard `Newlib` stubs to map `stdio` and system calls to internal Felix services and hardware drivers.
* **Cooperative Streaming Architecture:** (Planned) A microkernel design focused on piping data blocks between tasks and hardware interfaces without blocking.
* **Static VFS:** A virtual filesystem mapping `/dev/` and `/bin/` to static function trees and device drivers.
* **Interactive Shell:** The **`felix`** CLI, providing a robust, command-line interface for system debugging and control.
* **Multi-Platform Portability:** A clean separation between the kernel logic and the Hardware Abstraction Layer (HAL) for RISC-V and ARM architectures.

## 🛠️ Adding Custom Commands (The `argc`, `argv` approach)
One of Felix's most powerful features is how easily you can add new commands. You do not need to write complex kernel modules or learn a proprietary API.

Felix treats internal commands exactly like standard C executables. You just write a function using the standard POSIX signature (`int my_cmd(int argc, char **argv)`) and register it to the VFS using a single macro: `FLX_VFS_DECLARE_CMD`.

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
FLX_VFS_DECLARE_CMD(node_hello, "hello", &node_bin, cmd_hello, NULL, "Say hello");
```

Once mounted, the shell's standard `execvp` layer automatically finds it in `/bin/hello`, parses the user's quotes and variables via `wordexp`, and passes the resulting `argc` and `argv` directly to your function!

## 📂 Project Architecture
Felix OS is designed with a strict separation of concerns, heavily inspired by Linux and modern RTOS architectures. The core OS is entirely decoupled from the underlying hardware, allowing it to be easily ported across different architectures.
* `/arch`: Architecture-specific code, vendor HALs, and hardware drivers (e.g., `riscv64/polarfire`). This is the *only* place where hardware-specific registers and interrupts are accessed.
* `/docs`: Architectural documentation, build guides (e.g., SoftConsole setup), and format specs.
* `/drivers`: Hardware-agnostic OS drivers (e.g., the generic Serial/TTY pipeline). These manage ring buffers and backpressure, communicating with the `/arch` layer via standardized Operations Tables.
* `/external`: Third-party dependencies (e.g., `linenoise` for the REPL, `optparse`). Keeping these isolated makes license management and upstream merges easy.
* `/fs`: File system implementations (like `binfs`). Leverages the custom Felix Executable Format (FXF) for zero-cost compiled-in executables and **(planned)** dynamically loaded PIC-compiled apps.
* `/include`: Global OS headers, the Kernel API, and generic driver interfaces.
* `/kernel`: Core OS initialization, Virtual File System (VFS), Device Management, and **(planned)** the process scheduler and task management.
* `/posix`: Syscall implementations and standard C library "un-stubbing" (e.g., seamlessly mapping `printf` to the VFS and UART).
* `/shell`: The `felix` CLI, Linenoise REPL, delegated command autocompletion engine, and shell builtins.

## 📂 VFS Structure
Add as many directories, subdirectories, files and executables as your project needs.
* `/`: The root directory.
* `/bin`: A dynamic bin directory containing FXF executable payloads.
* `/dev`: (In process) A device directory mapping file descriptors to hardware (e.g., `stdin`, `stdout`).
* `/etc`: (Planned) A place for configuration files.
* `/test`: A home for Unit tests.

## ⚖️ The Cooperative Philosophy **(Planned)**
Unlike preemptive RTOSes, Felix operates on a **"Polite Citizen"** model.
* **No Preemption:** Tasks are never forcibly interrupted by the scheduler.
* **Explicit Yielding:** Performance and responsiveness are managed by the programmer using `yield()`, ensuring that context switches only happen at logical boundaries.
* **Deterministic I/O:** Ideal for streaming large data sets where timing and sequence are critical.