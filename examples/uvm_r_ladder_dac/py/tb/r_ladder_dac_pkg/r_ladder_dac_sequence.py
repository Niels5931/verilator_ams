from pyuvm import uvm_sequence

from .r_ladder_dac_seq_item import RLDacSeqItem


class RLDacRampSequence(uvm_sequence):
    """Send all 16 codes (0..15) to the sequencer."""

    async def body(self):
        for i in range(16):
            req = RLDacSeqItem("req")
            req.code = i
            await self.start_item(req)
            await self.finish_item(req)
