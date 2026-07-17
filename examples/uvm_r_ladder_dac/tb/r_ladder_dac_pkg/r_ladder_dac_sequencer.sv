class r_ladder_dac_sequencer extends uvm_sequencer#(r_ladder_dac_seq_item);
  `uvm_component_utils(r_ladder_dac_sequencer)

  function new(string name = "r_ladder_dac_sequencer", uvm_component parent = null);
    super.new(name, parent);
  endfunction
endclass
