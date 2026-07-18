import cocotb
from cocotb.triggers import RisingEdge

from pyuvm import uvm_test, ConfigDB, test

from ams import get_bridge

from .r_ladder_dac_pkg import RLDacEnv, RLDacRampSequence


@test()
class RLDacTest(uvm_test):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.vif = None
        self.env = None

    def build_phase(self):
        super().build_phase()
        # The py flow does not use an SV virtual interface; the top-level
        # DUT handle exposes the same signals directly.
        self.vif = cocotb.top
        ConfigDB().set(None, "*", "vif", self.vif)
        self.env = RLDacEnv("env", self)

    def start_of_simulation_phase(self):
        super().start_of_simulation_phase()
        get_bridge().init("config.yaml")

    async def run_phase(self):
        bridge = get_bridge()
        self.raise_objection()

        self.logger.info(f"VDD = {bridge.get_vdd():.6f}")

        seq = RLDacRampSequence("seq")
        await seq.start(self.env.agent.sequencer)

        # Allow the monitor and coverage collector to finish with the last
        # transaction before shutting down the AMS session.
        for _ in range(3):
            await RisingEdge(self.vif.clk)

        bridge.finish()

        self.drop_objection()
