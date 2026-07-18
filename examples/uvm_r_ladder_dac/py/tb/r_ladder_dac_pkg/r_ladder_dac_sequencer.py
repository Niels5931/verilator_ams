from pyuvm import uvm_sequencer


class RLDacSequencer(uvm_sequencer):
    def __init__(self, name, parent):
        super().__init__(name, parent)
