/**
 * @file felix/include/pty.h
 * @brief Minimal pseudoterminal shim to satisfy Toybox portability headers.
 */
#ifndef _PTY_H
#define _PTY_H

/* Felix OS does not currently support pseudoterminals (PTYs).
 * This file exists purely to allow external Linux utilities
 * to compile without throwing missing header errors. */

#endif /* _PTY_H */
