from pyuvm import uvm_agent

from .r_ladder_dac_sequencer import RLDacSequencer
from .r_ladder_dac_driver import RLDacDriver
from .r_ladder_dac_monitor import RLDacMonitor
from .r_ladder_dac_coverage import RLDacCoverage


class RLDacAgent(uvm_agent):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.sequencer = None
        self.driver = None
        self.monitor = None
        self.coverage = None

    def build_phase(self):
        super().build_phase()
        self.sequencer = RLDacSequencer("sequencer", self)
        self.driver = RLDacDriver("driver", self)
        self.monitor = RLDacMonitor("monitor", self)
        self.coverage = RLDacCoverage("coverage", self)

    def connect_phase(self):
        super().connect_phase()
        self.driver.seq_item_port.connect(self.sequencer.seq_item_export)
