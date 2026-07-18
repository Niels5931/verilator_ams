import cocotb
from cocotb.triggers import RisingEdge

from pyuvm import uvm_driver, uvm_analysis_port, ConfigDB

from ams import get_bridge

from .r_ladder_dac_result_item import RLDacResultItem


class RLDacDriver(uvm_driver):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.vif = None
        self.result_ap = None

    def build_phase(self):
        super().build_phase()
        self.vif = ConfigDB().get(self, "", "vif")
        self.result_ap = uvm_analysis_port("result_ap", self)

    async def run_phase(self):
        bridge = get_bridge()
        vdd = bridge.get_vdd()
        clock_period_ns = bridge.get_clock_period_ns()

        while True:
            req = await self.seq_item_port.get_next_item()

            # Wait for reset de-assertion.
            while self.vif.rst_n.value == 0:
                await RisingEdge(self.vif.clk)

            # Drive the new code at the rising edge.
            await RisingEdge(self.vif.clk)
            self.vif.code.value = req.code

            # Push the digital code into ngspice and advance the analog
            # simulation by one clock period.
            bridge.set_voltage("b0", vdd if req.code & 0x1 else 0.0)
            bridge.set_voltage("b1", vdd if req.code & 0x2 else 0.0)
            bridge.set_voltage("b2", vdd if req.code & 0x4 else 0.0)
            bridge.set_voltage("b3", vdd if req.code & 0x8 else 0.0)
            bridge.run_analog(clock_period_ns)

            # Sample the settled analog result and write it back to the
            # interface for VCD, then publish it to the monitor.
            vout = bridge.get_voltage("vout")
            self.logger.info(f"sampled vout={vout:.6f} for code={req.code}")
            self.vif.vout.value = int(vout * 65536.0)

            tr = RLDacResultItem("tr")
            tr.code = req.code
            tr.vout = vout
            self.result_ap.write(tr)

            self.seq_item_port.item_done()
