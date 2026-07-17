class r_ladder_dac_scoreboard extends uvm_scoreboard;
  `uvm_component_utils(r_ladder_dac_scoreboard)

  uvm_analysis_imp#(r_ladder_dac_result_item, r_ladder_dac_scoreboard) analysis_export;

  real tol = 0.05;

  function new(string name = "r_ladder_dac_scoreboard", uvm_component parent = null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    analysis_export = new("analysis_export", this);
  endfunction

  virtual function void write(r_ladder_dac_result_item tr);
    real vdd;
    if (tr == null) return;

    vdd = ams_dpi_pkg::ams_get_vdd();

    tr.expected = vdd * real'(tr.code) / 16.0;
    tr.passed   = (tr.vout >= tr.expected - tol) && (tr.vout <= tr.expected + tol);

    if (!tr.passed) begin
      `uvm_error("SCB",
        $sformatf("code=%0d vout=%f expected=%f FAIL",
                  tr.code, tr.vout, tr.expected))
    end else begin
      `uvm_info("SCB",
        $sformatf("code=%0d vout=%f expected=%f PASS",
                  tr.code, tr.vout, tr.expected),
        UVM_LOW)
    end
  endfunction
endclass
