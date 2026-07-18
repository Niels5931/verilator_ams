from pyuvm import uvm_env

from .r_ladder_dac_agent import RLDacAgent
from .r_ladder_dac_scoreboard import RLDacScoreboard


class RLDacEnv(uvm_env):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.agent = None
        self.scoreboard = None

    def build_phase(self):
        super().build_phase()
        self.agent = RLDacAgent("agent", self)
        self.scoreboard = RLDacScoreboard("scoreboard", self)

    def connect_phase(self):
        super().connect_phase()

        # The driver publishes sampled results to the monitor; the monitor
        # forwards them to the scoreboard and coverage collector.
        self.agent.driver.result_ap.connect(self.agent.monitor.analysis_export)
        self.agent.monitor.ap.connect(self.scoreboard.analysis_export)
        self.agent.monitor.ap.connect(self.agent.coverage.analysis_export)
