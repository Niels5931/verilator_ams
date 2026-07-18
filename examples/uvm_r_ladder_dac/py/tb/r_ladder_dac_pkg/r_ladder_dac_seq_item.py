from pyuvm import uvm_sequence_item


class RLDacSeqItem(uvm_sequence_item):
    def __init__(self, name="rl_dac_seq_item"):
        super().__init__(name)
        self.code = 0

    def __str__(self):
        return f"code={self.code}"
