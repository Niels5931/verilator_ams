"""ctypes binding to libams_core.so — the AMS bridge shared library.

This module provides a Python interface to the same AMS DPI-C functions
that SystemVerilog testbenches call via the DPI-C import package
(``include/ams/ams_dpi_pkg.sv``).  It is reusable across all pyuvm / cocotb
examples — no example-specific logic lives here.

Usage:
    from ams import get_bridge

    bridge = get_bridge()
    bridge.init("config.yaml")
    bridge.set_voltage("b0", vdd)
    bridge.run_analog(bridge.get_clock_period_ns())
    vout = bridge.get_voltage("vout")
    bridge.finish()
"""

import ctypes
import os
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent.parent.parent


class AMSBridge:
    """Thin ctypes wrapper around the C ``ams_*`` API in ``libams_core.so``."""

    def __init__(self, lib_path=None):
        if lib_path is None:
            lib_path = str(_REPO_ROOT / "build" / "core" / "libams_core.so")
        self._lib = ctypes.CDLL(lib_path)
        self._bind()

    def _bind(self):
        lib = self._lib
        # int ams_init(const char* config_path)
        lib.ams_init.restype = ctypes.c_int
        lib.ams_init.argtypes = [ctypes.c_char_p]
        # int ams_finish(void)
        lib.ams_finish.restype = ctypes.c_int
        lib.ams_finish.argtypes = []
        # int ams_run_analog(double dt_ns)
        lib.ams_run_analog.restype = ctypes.c_int
        lib.ams_run_analog.argtypes = [ctypes.c_double]
        # double ams_get_voltage(const char* node_name)
        lib.ams_get_voltage.restype = ctypes.c_double
        lib.ams_get_voltage.argtypes = [ctypes.c_char_p]
        # int ams_set_voltage(const char* source_name, double voltage)
        lib.ams_set_voltage.restype = ctypes.c_int
        lib.ams_set_voltage.argtypes = [ctypes.c_char_p, ctypes.c_double]
        # int ams_open_trace(const char* filename)
        lib.ams_open_trace.restype = ctypes.c_int
        lib.ams_open_trace.argtypes = [ctypes.c_char_p]
        # int ams_close_trace(void)
        lib.ams_close_trace.restype = ctypes.c_int
        lib.ams_close_trace.argtypes = []
        # double ams_get_vdd(void)
        lib.ams_get_vdd.restype = ctypes.c_double
        lib.ams_get_vdd.argtypes = []
        # double ams_get_clock_period_ns(void)
        lib.ams_get_clock_period_ns.restype = ctypes.c_double
        lib.ams_get_clock_period_ns.argtypes = []
        # double ams_get_sim_time_ns(void)
        lib.ams_get_sim_time_ns.restype = ctypes.c_double
        lib.ams_get_sim_time_ns.argtypes = []

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def init(self, config_path: str) -> int:
        """Load the YAML config and start ngspice.  Returns 0 on success."""
        return self._lib.ams_init(config_path.encode())

    def finish(self) -> int:
        """Tear down the ngspice session."""
        return self._lib.ams_finish()

    # ------------------------------------------------------------------
    # Analog co-simulation
    # ------------------------------------------------------------------

    def run_analog(self, dt_ns: float) -> int:
        """Advance the analog simulation by *dt_ns* nanoseconds."""
        return self._lib.ams_run_analog(dt_ns)

    def get_voltage(self, node_name: str) -> float:
        """Read the voltage at *node_name* from ngspice."""
        return self._lib.ams_get_voltage(node_name.encode())

    def set_voltage(self, source_name: str, voltage: float) -> int:
        """Set the ngspice EXTERNAL voltage source *source_name*."""
        return self._lib.ams_set_voltage(source_name.encode(), voltage)

    # ------------------------------------------------------------------
    # Trace
    # ------------------------------------------------------------------

    def open_trace(self, filename: str) -> int:
        return self._lib.ams_open_trace(filename.encode())

    def close_trace(self) -> int:
        return self._lib.ams_close_trace()

    # ------------------------------------------------------------------
    # Configuration queries
    # ------------------------------------------------------------------

    def get_vdd(self) -> float:
        return self._lib.ams_get_vdd()

    def get_clock_period_ns(self) -> float:
        return self._lib.ams_get_clock_period_ns()

    def get_sim_time_ns(self) -> float:
        return self._lib.ams_get_sim_time_ns()


# Module-level singleton (mirrors ams::ams_active_testbench on the C side).
_bridge: AMSBridge | None = None


def get_bridge(lib_path: str | None = None) -> AMSBridge:
    """Return the process-wide AMS bridge singleton."""
    global _bridge
    if _bridge is None:
        _bridge = AMSBridge(lib_path)
    return _bridge
