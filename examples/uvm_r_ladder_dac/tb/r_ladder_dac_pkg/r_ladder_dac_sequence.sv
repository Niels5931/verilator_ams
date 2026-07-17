class r_ladder_dac_ramp_sequence extends uvm_sequence#(r_ladder_dac_seq_item);
  `uvm_object_utils(r_ladder_dac_ramp_sequence)

  function new(string name = "r_ladder_dac_ramp_sequence");
    super.new(name);
  endfunction

  virtual task body();
    r_ladder_dac_seq_item req;

    for (int i = 0; i < 16; i++) begin
      req = r_ladder_dac_seq_item::type_id::create("req");
      start_item(req);
      if (!req.randomize() with { req.code == i; }) begin
        `uvm_error("SEQ", "Failed to randomize sequence item")
      end
      finish_item(req);
    end
  endtask
endclass
