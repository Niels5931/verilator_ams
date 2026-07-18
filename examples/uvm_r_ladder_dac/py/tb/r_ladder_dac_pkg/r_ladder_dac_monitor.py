from pyuvm import uvm_subscriber, uvm_analysis_port

from .r_ladder_dac_result_item import RLDacResultItem


class RLDacMonitor(uvm_subscriber):
    """Monitor receives the sampled result from the driver and forwards it.

    In the pyuvm/cocotb flow the driver pushes the digital code into ngspice and
    samples the settled analog voltage.  It publishes the result directly to
    this monitor, which then forwards it to the scoreboard and coverage.  This
    avoids the timing races that occur when blocking C-library calls are made
    from a cocotb coroutine while the Verilog clock continues to advance.
    """

    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.ap = None

    def build_phase(self):
        super().build_phase()
        self.ap = uvm_analysis_port("ap", self)

    def write(self, tr):
        if tr is None:
            return
        self.logger.info(f"monitored code={tr.code} vout={tr.vout:.6f}")
        self.ap.write(tr)
