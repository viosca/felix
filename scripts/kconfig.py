#!/usr/bin/env python3
"""
Felix OS Kconfig Automation Runner
Manages .config and autoconf.h generation using vendored kconfiglib.
"""

import sys
import os

# Locate script and felix root directories
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FELIX_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
WORKSPACE_ROOT = os.path.abspath(os.path.join(FELIX_ROOT, ".."))

# Add vendored kconfiglib to Python path
sys.path.insert(0, os.path.join(SCRIPT_DIR, "kconfig"))

try:
    import kconfiglib
except ImportError:
    sys.exit("Error: kconfiglib not found in felix/scripts/kconfig/")

KCONFIG_FILE = os.path.join(FELIX_ROOT, "Kconfig")
DOT_CONFIG = os.path.join(WORKSPACE_ROOT, ".config")
AUTOCONF_HEADER = os.path.join(FELIX_ROOT, "include", "generated", "autoconf.h")
CONFIGS_DIR = os.path.join(FELIX_ROOT, "configs")


def ensure_dirs():
    os.makedirs(os.path.dirname(AUTOCONF_HEADER), exist_ok=True)
    os.makedirs(CONFIGS_DIR, exist_ok=True)


def generate_header(kconf):
    ensure_dirs()
    kconf.write_autoconf(AUTOCONF_HEADER)
    print(f"[Kconfig] Generated C header: {os.path.relpath(AUTOCONF_HEADER, WORKSPACE_ROOT)}")


def cmd_defconfig(defconfig_arg):
    ensure_dirs()
    # Resolve defconfig path
    if os.path.isabs(defconfig_arg):
        path = defconfig_arg
    elif os.path.exists(os.path.join(CONFIGS_DIR, defconfig_arg)):
        path = os.path.join(CONFIGS_DIR, defconfig_arg)
    elif os.path.exists(defconfig_arg):
        path = defconfig_arg
    else:
        sys.exit(f"Error: Defconfig not found: {defconfig_arg}")

    kconf = kconfiglib.Kconfig(KCONFIG_FILE)
    kconf.load_config(path)
    kconf.write_config(DOT_CONFIG)
    print(f"[Kconfig] Loaded defconfig: {os.path.basename(path)}")
    print(f"[Kconfig] Written configuration: {os.path.relpath(DOT_CONFIG, WORKSPACE_ROOT)}")
    generate_header(kconf)


def cmd_genheader():
    ensure_dirs()
    if not os.path.exists(DOT_CONFIG):
        print(f"[Kconfig] No .config found, loading defaults from Kconfig...")
        kconf = kconfiglib.Kconfig(KCONFIG_FILE)
        kconf.write_config(DOT_CONFIG)
    else:
        kconf = kconfiglib.Kconfig(KCONFIG_FILE)
        kconf.load_config(DOT_CONFIG)

    generate_header(kconf)


def cmd_menuconfig():
    import menuconfig
    # Set environment variables for menuconfig
    os.environ["KCONFIG_CONFIG"] = DOT_CONFIG
    sys.argv = [sys.argv[0], KCONFIG_FILE]
    menuconfig._main()
    # After menuconfig closes, update autoconf.h
    cmd_genheader()


def cmd_guiconfig():
    import guiconfig
    # Set environment variables for guiconfig
    os.environ["KCONFIG_CONFIG"] = DOT_CONFIG
    sys.argv = [sys.argv[0], KCONFIG_FILE]
    guiconfig._main()
    # After guiconfig closes, update autoconf.h
    cmd_genheader()


def print_help():
    print("""Felix OS Kconfig Helper

Usage:
    python felix/scripts/kconfig.py <command> [args]

Commands:
    defconfig <name>    Load a predefined configuration (e.g. disco_ddr_defconfig)
    genheader           Regenerate include/generated/autoconf.h from current .config
    menuconfig          Interactive terminal configuration menu (ncurses)
    guiconfig           Graphical configuration editor (Tk GUI)
    help                Show this help message
""")


def main():
    if len(sys.argv) < 2:
        print_help()
        sys.exit(1)

    cmd = sys.argv[1].lower()

    if cmd == "defconfig":
        if len(sys.argv) < 3:
            sys.exit("Error: defconfig requires a configuration name or path.")
        cmd_defconfig(sys.argv[2])
    elif cmd in ("genheader", "sync"):
        cmd_genheader()
    elif cmd == "menuconfig":
        cmd_menuconfig()
    elif cmd in ("guiconfig", "gui"):
        cmd_guiconfig()
    elif cmd in ("--help", "-h", "help"):
        print_help()
    else:
        sys.exit(f"Unknown command: {cmd}")


if __name__ == "__main__":
    main()
