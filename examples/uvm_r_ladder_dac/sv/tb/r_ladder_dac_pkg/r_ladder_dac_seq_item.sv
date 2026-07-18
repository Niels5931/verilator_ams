class r_ladder_dac_seq_item extends uvm_sequence_item;
  `uvm_object_utils(r_ladder_dac_seq_item)

  rand bit [3:0] code;

  function new(string name = "r_ladder_dac_seq_item");
    super.new(name);
  endfunction

  virtual function string convert2string();
    return $sformatf("code=%0d", code);
  endfunction
endclass
