from pyuvm import uvm_subscriber


class RLDacCoverage(uvm_subscriber):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.code_bins = set()
        self.vout_bins = set()

    def write(self, t):
        if t is None:
            return
        self.code_bins.add(t.code)
        vout_bin = int(t.vout * 1000.0) // 100
        self.vout_bins.add(vout_bin)

    def report_phase(self):
        super().report_phase()
        self.logger.info(
            f"Coverage: {len(self.code_bins)}/16 codes hit, "
            f"{len(self.vout_bins)} vout bins hit"
        )
