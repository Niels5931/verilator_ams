class r_ladder_dac_coverage extends uvm_subscriber#(r_ladder_dac_result_item);
  `uvm_component_utils(r_ladder_dac_coverage)

  r_ladder_dac_result_item cur;
  int                        vout_bin;

  covergroup cg;
    option.per_instance = 1;

    code_cp: coverpoint cur.code {
      bins c[] = {[0:15]};
    }

    vout_cp: coverpoint vout_bin {
      bins mv[] = {[0:18]};
    }
  endgroup

  function new(string name = "r_ladder_dac_coverage", uvm_component parent = null);
    super.new(name, parent);
    cg = new();
  endfunction

  virtual function void write(r_ladder_dac_result_item t);
    if (t == null) return;
    cur      = t;
    vout_bin = int'(t.vout * 1000.0) / 100;
    cg.sample();
  endfunction
endclass
