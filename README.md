# Felix OS

Felix OS is a lightweight, bare-metal, POSIX-compliant operating system built from scratch for the RISC-V 64-bit architecture.

## The Goal

The primary goal of Felix OS is to provide a fully functional, cooperative multitasking environment with a robust Virtual File System (VFS) and standard POSIX interface, while maintaining an incredibly lean memory footprint (currently running in under 150 KB of ROM and 100 KB of RAM). It aims to bridge the gap between a simple RTOS and a full-scale Unix-like system.

## Progress & Features

Felix OS is actively in development. The current milestones achieved include:

* **RISC-V 64-bit Core:** Custom hardware initialization, lightweight memory management (bump allocator via `sbrk`), and standard timer/clock integration.

* **POSIX Compliance:** Deep integration with Newlib (Libgloss backend) featuring a thread-safe reentrancy architecture (`struct _reent`), standard file descriptor routing, and isolated environment variables.

* **Virtual File System (VFS):** A modular filesystem hierarchy currently supporting static memory-mapped filesystems (`/bin`, `/dev`, `/etc`) with full support for standard I/O and directory streaming (`opendir`, `readdir`).

* **Interactive Shell:** A rich, interactive command-line interface powered by `linenoise`, featuring dynamic history, tab-autocompletion, and `$PATH` resolution.

* **Executable Binaries:** Isolated user-space tools loaded as discrete linker sections (including `ls`, `hexdump`, `date`, `echo`, `which`, and `clear`).

* **Kernel Gatekeeping:** A strict system call boundary (`sys_*`) safely decoupling user-space POSIX functions from core kernel operations.

## Up Next

* Developing a `TmpFS` (RAM Disk) to mount at `/tmp` for dynamic file creation and writing.

* Transitioning the integrated shell into a standalone user-space executable (`/bin/sh`).

* Implementing true cooperative multitasking and hardware context switching.