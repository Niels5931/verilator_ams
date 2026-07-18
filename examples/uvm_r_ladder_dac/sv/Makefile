# Makefile for the SystemVerilog UVM R-ladder DAC AMS co-simulation.
# Run:  make
#
# Required environment variables (see README):
#   UVM_ROOT        path to uvm-verilator source tree (contains uvm_pkg.sv)
#   NGSPICE_HOME    ngspice install root (has include/ and lib/)
#   YAML_CPP_HOME   yaml-cpp install root (has include/ and lib/)
# Optional:
#   VERILATOR       verilator binary (default: `verilator` on PATH)
#   VERILATOR_ROOT  verilator install root (auto-detected from the binary)

.DEFAULT_GOAL := sim

VERILATOR      ?= verilator
VERILATOR_ROOT ?= $(shell $(VERILATOR) --getenv VERILATOR_ROOT 2>/dev/null)

REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

# --- Required user-supplied paths -------------------------------------------------
# Fail fast if any required env var is missing.  Skipped for `clean` so you can
# wipe a broken build without setting up the environment.  See README for what
# each path must contain (UVM_ROOT has uvm_pkg.sv; NGSPICE_HOME/YAML_CPP_HOME
# have include/ and lib/ subdirs).
ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifndef UVM_ROOT
$(error UVM_ROOT must point to the uvm-verilator source tree; see README)
endif
ifndef NGSPICE_HOME
$(error NGSPICE_HOME must point to the ngspice install root; see README)
endif
ifndef YAML_CPP_HOME
$(error YAML_CPP_HOME must point to the yaml-cpp install root; see README)
endif
endif

TOP    := ams_top
PREFIX := Vams_top
SIM_BUILD := $(CURDIR)/sim_build

VERILOG_SOURCES = \
    $(CURDIR)/verilog/r_ladder_dac_if.sv \
    $(CURDIR)/verilog/r_ladder_dac_dut.sv \
    $(CURDIR)/tb/r_ladder_dac_pkg/r_ladder_dac_pkg.sv \
    $(CURDIR)/verilog/ams_top.sv \
    $(CURDIR)/tb/r_ladder_dac_test.sv

VERILATOR_ARGS = \
    --trace --assert --timing -Wno-fatal \
    +define+UVM_NO_DPI \
    +incdir+$(UVM_ROOT) \
    +incdir+$(REPO_ROOT)/include/ams \
    +incdir+$(CURDIR)/tb/r_ladder_dac_pkg

# Flags handed to the C++ compiler Verilator drives.
CFLAGS = \
    -I$(REPO_ROOT)/include \
    -I$(VERILATOR_ROOT)/include/vltstd \
    -I$(NGSPICE_HOME)/include \
    -I$(YAML_CPP_HOME)/include

AMS_CORE_DIR := $(REPO_ROOT)/build/core

LDFLAGS = \
    -L$(AMS_CORE_DIR) -lams_core -Wl,-rpath,$(AMS_CORE_DIR) \
    -L$(NGSPICE_HOME)/lib -lngspice \
    -L$(YAML_CPP_HOME)/lib -lyaml-cpp \
    -lpthread -ldl

# --- Auto-build libams_core.so ----------------------------------------------------
AMS_CORE_LIB  := $(AMS_CORE_DIR)/libams_core.so
AMS_SIM       := $(REPO_ROOT)/tools/ams_sim.py
AMS_CORE_SRCS := $(REPO_ROOT)/src/ngspice_interface.cpp \
                 $(REPO_ROOT)/src/ams_dpi.cpp

$(AMS_CORE_LIB): $(AMS_SIM) $(AMS_CORE_SRCS)
	@echo "=== Building libams_core.so ==="
	python3 $(AMS_SIM) build-core

# --- Simulator binary ------------------------------------------------------------
SIM_EXE := $(SIM_BUILD)/$(PREFIX)

$(SIM_BUILD):
	mkdir -p $(SIM_BUILD)

# Verilator's generated Makefile does not track external shared libs as deps;
# listing $(AMS_CORE_LIB) as a prerequisite forces a relink when it changes.
$(SIM_EXE): $(AMS_CORE_LIB) $(VERILOG_SOURCES) | $(SIM_BUILD)
	$(VERILATOR) --binary \
	    --top-module $(TOP) --prefix $(PREFIX) \
	    -Mdir $(SIM_BUILD) \
	    $(VERILATOR_ARGS) \
	    $(foreach f,$(CFLAGS),-CFLAGS $(f)) \
	    $(foreach f,$(LDFLAGS),-LDFLAGS $(f)) \
	    $(VERILOG_SOURCES)

# --- Targets ----------------------------------------------------------------------
sim: $(SIM_EXE)
	cd $(CURDIR) && ./$(PREFIX) config.yaml

run: sim

clean:
	rm -rf $(SIM_BUILD)