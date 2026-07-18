from pyuvm import uvm_subscriber

from ams import get_bridge


class RLDacScoreboard(uvm_subscriber):
    """Scoreboard implemented as a subscriber so it can receive transactions."""

    tol = 0.05

    def __init__(self, name, parent):
        super().__init__(name, parent)

    def write(self, tr):
        if tr is None:
            return
        vdd = get_bridge().get_vdd()
        tr.expected = vdd * tr.code / 16.0
        tr.passed = abs(tr.vout - tr.expected) <= self.tol

        if not tr.passed:
            self.logger.error(
                f"code={tr.code} vout={tr.vout:.6f} "
                f"expected={tr.expected:.6f} FAIL"
            )
        else:
            self.logger.info(
                f"code={tr.code} vout={tr.vout:.6f} "
                f"expected={tr.expected:.6f} PASS"
            )
