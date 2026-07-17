class r_ladder_dac_result_item extends uvm_sequence_item;
  `uvm_object_utils(r_ladder_dac_result_item)

  bit  [3:0] code;
  real       vout;
  real       expected;
  bit        passed;

  function new(string name = "r_ladder_dac_result_item");
    super.new(name);
  endfunction

  virtual function string convert2string();
    return $sformatf("code=%0d vout=%f expected=%f %s",
                     code, vout, expected, passed ? "PASS" : "FAIL");
  endfunction
endclass
