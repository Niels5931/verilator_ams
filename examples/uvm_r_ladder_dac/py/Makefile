# cocotb Makefile for pyuvm R-ladder DAC co-simulation.
# Run:  make SIM=verilator

.DEFAULT_GOAL := sim

SIM ?= verilator
TOPLEVEL_LANG ?= verilog

# Repo root derived from this Makefile's own location (so `make -C .../py`
# from anywhere resolves correctly).  py/ is three levels below repo root.
REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

# Let cocotb's Python import the reusable ctypes binding (tools/ams_py) and
# the example's own test module (py/ on sys.path so `tb.r_ladder_dac_test`
# resolves).
PYTHONPATH := $(REPO_ROOT)/tools/ams_py:$(CURDIR)$(if $(PYTHONPATH),:$(PYTHONPATH))
export PYTHONPATH

# libams_core.so is loaded by ctypes via an absolute path, but prepending
# build/core here also aids its transitive shared-library deps in unusual
# installs where the user doesn't want to touch the shell environment.
LD_LIBRARY_PATH := $(REPO_ROOT)/build/core$(if $(LD_LIBRARY_PATH),:$(LD_LIBRARY_PATH))
export LD_LIBRARY_PATH

VERILOG_SOURCES = $(CURDIR)/verilog/ams_top.sv \
                  $(CURDIR)/../sv/verilog/r_ladder_dac_dut.sv

TOPLEVEL = ams_top
MODULE = tb.r_ladder_dac_test

EXTRA_ARGS += --trace --assert --timing

# Auto-build libams_core.so if missing or stale.  Hook it through cocotb's
# CUSTOM_SIM_DEPS so it runs before the simulation target.  `build-core` only
# shells out to c++; it does not require cocotb.
AMS_CORE_LIB  := $(REPO_ROOT)/build/core/libams_core.so
AMS_SIM       := $(REPO_ROOT)/tools/ams_sim.py
AMS_CORE_SRCS := $(REPO_ROOT)/src/ngspice_interface.cpp \
                 $(REPO_ROOT)/src/ams_dpi.cpp

$(AMS_CORE_LIB): $(AMS_SIM) $(AMS_CORE_SRCS)
	@echo "=== Building libams_core.so ==="
	python3 $(AMS_SIM) build-core

CUSTOM_SIM_DEPS += $(AMS_CORE_LIB)

# Let the Makefile locate cocotb's makefiles without requiring cocotb-config
# to be on the shell PATH.  `run.sh` can still override by exporting
# COCOTB_MAKEFILES, since we use `?=`.
COCOTB_MAKEFILES ?= $(shell python3 -m cocotb_tools.config --makefiles)

include $(COCOTB_MAKEFILES)/Makefile.sim
