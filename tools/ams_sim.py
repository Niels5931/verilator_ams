#!/usr/bin/env python3
"""Build driver for the AMS co-simulation core library.

The only job of this script is to build ``build/core/libams_core.so`` from
``src/ngspice_interface.cpp`` and ``src/ams_dpi.cpp``, linking the system
ngspice + yaml-cpp.  Each example owns its own Makefile for the Verilator
(SV UVM flow) or cocotb (pyuvm flow) build and run.

Usage:
    ./tools/ams_sim.py build-core
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_ROOT = REPO_ROOT / "build"


def run_cmd(cmd, cwd=None, env=None, check=True):
    """Run a shell command and print it."""
    cmd_str = " ".join(str(c) for c in cmd)
    print(f"[AMS] {cmd_str}")
    result = subprocess.run(cmd, cwd=cwd, env=env, check=False)
    if check and result.returncode != 0:
        raise RuntimeError(f"Command failed with exit code {result.returncode}")
    return result


def pkg_config(name):
    """Return (cflags, libs) from pkg-config if available."""
    if shutil.which("pkg-config") is None:
        return [], []
    try:
        cflags = subprocess.check_output(["pkg-config", "--cflags", name], text=True).split()
    except subprocess.CalledProcessError:
        cflags = []
    try:
        libs = subprocess.check_output(["pkg-config", "--libs", name], text=True).split()
    except subprocess.CalledProcessError:
        libs = []
    return cflags, libs


def find_ngspice():
    """Return (include_dirs, lib_dirs, libs) for ngspice."""
    home = os.environ.get("NGSPICE_HOME")
    if home:
        inc = Path(home) / "include"
        lib = Path(home) / "lib"
        if (inc / "ngspice" / "sharedspice.h").exists():
            return [str(inc)], [str(lib)], ["-lngspice"]

    # Try pkg-config.
    cflags, libs = pkg_config("ngspice")
    if libs:
        inc_dirs = [f[2:] for f in cflags if f.startswith("-I")]
        lib_dirs = [f[2:] for f in libs if f.startswith("-L")]
        lib_flags = [f for f in libs if not f.startswith("-L")]
        return inc_dirs, lib_dirs, lib_flags

    # Common system locations.
    for lib_name in ["ngspice", "ngspice-0"]:
        for lib_dir in ["/usr/lib", "/usr/local/lib", "/usr/lib/x86_64-linux-gnu"]:
            so = Path(lib_dir) / f"lib{lib_name}.so"
            if so.exists():
                for inc_dir in ["/usr/include", "/usr/local/include"]:
                    if (Path(inc_dir) / "ngspice" / "sharedspice.h").exists():
                        return [inc_dir], [lib_dir], [f"-l{lib_name}"]

    raise RuntimeError(
        "ngspice development library not found. "
        "Set NGSPICE_HOME or install libngspice / ngspice-devel."
    )


def find_yaml_cpp():
    """Return (include_dirs, lib_dirs, libs) for yaml-cpp."""
    home = os.environ.get("YAML_CPP_HOME")
    if home:
        inc = Path(home) / "include"
        lib = Path(home) / "lib"
        if (inc / "yaml-cpp" / "yaml.h").exists():
            return [str(inc)], [str(lib)], ["-lyaml-cpp"]

    cflags, libs = pkg_config("yaml-cpp")
    if libs:
        inc_dirs = [f[2:] for f in cflags if f.startswith("-I")]
        lib_dirs = [f[2:] for f in libs if f.startswith("-L")]
        lib_flags = [f for f in libs if not f.startswith("-L")]
        return inc_dirs, lib_dirs, lib_flags

    for lib_dir in ["/usr/lib", "/usr/local/lib", "/usr/lib/x86_64-linux-gnu"]:
        so = Path(lib_dir) / "libyaml-cpp.so"
        if so.exists():
            for inc_dir in ["/usr/include", "/usr/local/include"]:
                if (Path(inc_dir) / "yaml-cpp" / "yaml.h").exists():
                    return [inc_dir], [lib_dir], ["-lyaml-cpp"]

    raise RuntimeError(
        "yaml-cpp library not found. Install libyaml-cpp-dev or set YAML_CPP_HOME."
    )


def get_cxx():
    return os.environ.get("CXX", "c++")


def discover_deps():
    """Discover the tools needed to build libams_core.so only."""
    return {
        "cxx": get_cxx(),
        "ngspice_inc": find_ngspice(),
        "yaml_cpp": find_yaml_cpp(),
    }


def build_core_lib(deps, force=False):
    """Build libams_core.so once and reuse it across examples."""
    core_dir = BUILD_ROOT / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    lib = core_dir / "libams_core.so"

    if lib.exists() and not force:
        return lib

    ng_inc, ng_lib, ng_libs = deps["ngspice_inc"]
    yc_inc, yc_lib, yc_libs = deps["yaml_cpp"]

    inc_flags = []
    for d in ng_inc + yc_inc + [str(REPO_ROOT / "include")]:
        inc_flags.extend(["-I", d])

    ld_flags = []
    for d in ng_lib + yc_lib:
        ld_flags.extend(["-L", d])
    ld_flags.extend(ng_libs)
    ld_flags.extend(yc_libs)
    ld_flags.extend(["-lpthread", "-ldl"])

    run_cmd([
        deps["cxx"], "-std=c++17", "-fcoroutines", "-fPIC", "-O2", "-shared",
        *inc_flags,
        str(REPO_ROOT / "src" / "ngspice_interface.cpp"),
        str(REPO_ROOT / "src" / "ams_dpi.cpp"),
        *ld_flags,
        "-o", str(lib),
    ])
    return lib


def main():
    parser = argparse.ArgumentParser(
        description="Build driver for the AMS co-simulation core library (libams_core.so). "
                    "Each example has its own Makefile for the Verilator/cocotb build and run.",
    )
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    core_parser = subparsers.add_parser("build-core", help="Build libams_core.so")
    core_parser.add_argument("--force", action="store_true", help="Force rebuild")

    args = parser.parse_args()

    if args.cmd == "build-core":
        deps = discover_deps()
        lib = build_core_lib(deps, force=args.force)
        print(f"[AMS] libams_core.so: {lib}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as e:
        print(f"[AMS] Error: {e}", file=sys.stderr)
        sys.exit(1)
