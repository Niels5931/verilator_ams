#!/usr/bin/env python3
"""AMS build and run driver for Verilator + UVM + ngspice cosimulation.

This script replaces per-example CMakeLists.txt files.  Each example provides a
bench.yaml manifest; the driver discovers tools, builds a shared AMS core
library once, verilates the example, and links the final simulator.

Usage:
    ./tools/ams_sim.py build uvm_r_ladder_dac
    ./tools/ams_sim.py run   uvm_r_ladder_dac
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


def find_program(name):
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"Required program not found: {name}")
    return Path(path)


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


def find_uvm():
    root = os.environ.get("UVM_ROOT")
    if not root:
        raise RuntimeError("UVM_ROOT environment variable not set")
    pkg = Path(root) / "uvm_pkg.sv"
    if not pkg.exists():
        raise RuntimeError(f"UVM package not found at {pkg}")
    return Path(root)


def get_cxx():
    return os.environ.get("CXX", "c++")


def discover_deps():
    return {
        "verilator": find_program("verilator"),
        "cxx": get_cxx(),
        "uvm_root": find_uvm(),
        "ngspice_inc": find_ngspice(),
        "yaml_cpp": find_yaml_cpp(),
    }


def build_core_lib(deps, force=False):
    """Build libams_lib.a once and reuse it across examples."""
    core_dir = BUILD_ROOT / "core"
    core_dir.mkdir(parents=True, exist_ok=True)
    lib = core_dir / "libams_lib.a"

    if lib.exists() and not force:
        return lib

    ng_inc, ng_lib, ng_libs = deps["ngspice_inc"]
    yc_inc, yc_lib, yc_libs = deps["yaml_cpp"]

    inc_flags = []
    for d in ng_inc + yc_inc + [str(REPO_ROOT / "include")]:
        inc_flags.extend(["-I", d])

    objects = []
    for src in ["src/ngspice_interface.cpp", "src/ams_dpi.cpp"]:
        obj = core_dir / (Path(src).stem + ".o")
        objects.append(obj)
        run_cmd([
            deps["cxx"], "-std=c++17", "-fPIC", "-O2", "-c",
            *inc_flags,
            str(REPO_ROOT / src),
            "-o", str(obj),
        ])

    run_cmd(["ar", "rcs", str(lib), *map(str, objects)])
    return lib


def load_bench(example):
    import tomllib
    bench_path = REPO_ROOT / "examples" / example / "bench.toml"
    if not bench_path.exists():
        raise RuntimeError(f"bench.toml not found for example: {example}")
    with bench_path.open("rb") as f:
        return tomllib.load(f)


def resolve_sources(example_dir, bench, deps):
    """Return absolute paths of all Verilog/SystemVerilog sources."""
    sources = []
    if bench.get("uvm"):
        sources.append(deps["uvm_root"] / "uvm_pkg.sv")
        sources.append(REPO_ROOT / "include" / "ams" / "ams_dpi_pkg.sv")
    for rel in bench["verilog"]:
        src = example_dir / rel
        if not src.exists():
            raise RuntimeError(f"Verilog source not found: {src}")
        sources.append(src)
    return sources


def verilate(example_dir, bench, build_dir, deps):
    """Run Verilator to generate C++ model."""
    verilator_dir = build_dir / "verilator"
    verilator_dir.mkdir(parents=True, exist_ok=True)

    top = bench["top"]
    prefix = bench.get("prefix", f"V{top}")
    uvm_root = deps["uvm_root"]
    ams_inc = REPO_ROOT / "include" / "ams"

    cmd = [
        str(deps["verilator"]),
        "--cc",
        "--top-module", top,
        "--prefix", prefix,
        "-Mdir", str(verilator_dir),
        "--trace",
        "--assert",
        "--timing",
        "-Wno-fatal",
        f"+incdir+{uvm_root}",
        f"+incdir+{ams_inc}",
        "+define+UVM_NO_DPI",
    ]

    for arg in bench.get("verilator_args", []):
        cmd.append(arg)

    for src in resolve_sources(example_dir, bench, deps):
        cmd.append(str(src))

    run_cmd(cmd, cwd=example_dir)
    return verilator_dir, prefix


def compile_example(example_dir, bench, build_dir, deps, core_lib):
    """Compile main.cpp + Verilated sources and link the simulator."""
    top = bench["top"]
    prefix = bench.get("prefix", f"V{top}")
    verilator_dir = build_dir / "verilator"

    ng_inc, ng_lib, ng_libs = deps["ngspice_inc"]
    yc_inc, yc_lib, yc_libs = deps["yaml_cpp"]

    inc_flags = ["-I", str(verilator_dir)]
    for d in [str(REPO_ROOT / "include"), str(deps["verilator"].parent / "share" / "verilator" / "include")] + ng_inc + yc_inc:
        inc_flags.extend(["-I", d])

    lib_flags = [str(core_lib)]
    for d in ng_lib + yc_lib:
        lib_flags.extend(["-L", d])
    lib_flags.extend(ng_libs)
    lib_flags.extend(yc_libs)
    lib_flags.extend(["-lpthread", "-ldl"])

    objects = []

    # Compile main.cpp
    main_obj = build_dir / "main.o"
    objects.append(main_obj)
    run_cmd([
        deps["cxx"], "-std=c++17", "-O2", "-c",
        *inc_flags,
        str(example_dir / "main.cpp"),
        "-o", str(main_obj),
    ])

    # Compile Verilated sources.
    for src in [f"{prefix}.cpp", f"{prefix}__Trace__0.cpp", f"{prefix}__Trace__0__Slow.cpp",
                f"{prefix}__Syms.cpp"]:
        src_path = verilator_dir / src
        if not src_path.exists():
            continue
        obj = build_dir / (src.stem + ".o")
        objects.append(obj)
        run_cmd([
            deps["cxx"], "-std=c++17", "-O2", "-c",
            *inc_flags,
            str(src_path),
            "-o", str(obj),
        ])

    # Link.
    exe = build_dir / f"ams_sim_{bench['name']}"
    run_cmd([deps["cxx"], "-std=c++17", *map(str, objects), *lib_flags, "-o", str(exe)])
    return exe


def build_example(example, force_core=False):
    example_dir = REPO_ROOT / "examples" / example
    bench = load_bench(example)
    build_dir = BUILD_ROOT / example
    build_dir.mkdir(parents=True, exist_ok=True)

    deps = discover_deps()
    core_lib = build_core_lib(deps, force=force_core)
    verilate(example_dir, bench, build_dir, deps)
    exe = compile_example(example_dir, bench, build_dir, deps, core_lib)
    print(f"[AMS] Built: {exe}")
    return exe


def run_example(example):
    bench = load_bench(example)
    exe = BUILD_ROOT / example / f"ams_sim_{bench['name']}"
    if not exe.exists():
        exe = build_example(example)
    example_dir = REPO_ROOT / "examples" / example
    config = bench.get("config", "config.yaml")
    run_cmd([str(exe), config], cwd=example_dir)


def main():
    parser = argparse.ArgumentParser(description="AMS Verilator + UVM + ngspice driver")
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    build_parser = subparsers.add_parser("build", help="Build an example")
    build_parser.add_argument("example")
    build_parser.add_argument("--force-core", action="store_true", help="Rebuild AMS core library")

    run_parser = subparsers.add_parser("run", help="Build if needed and run an example")
    run_parser.add_argument("example")

    args = parser.parse_args()

    if args.cmd == "build":
        build_example(args.example, force_core=args.force_core)
    elif args.cmd == "run":
        run_example(args.example)


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as e:
        print(f"[AMS] Error: {e}", file=sys.stderr)
        sys.exit(1)
