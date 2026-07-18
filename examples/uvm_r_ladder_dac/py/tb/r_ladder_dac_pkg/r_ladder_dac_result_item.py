from pyuvm import uvm_sequence_item


class RLDacResultItem(uvm_sequence_item):
    def __init__(self, name="rl_dac_result_item"):
        super().__init__(name)
        self.code = 0
        self.vout = 0.0
        self.expected = 0.0
        self.passed = False

    def __str__(self):
        status = "PASS" if self.passed else "FAIL"
        return f"code={self.code} vout={self.vout:.6f} expected={self.expected:.6f} {status}"
