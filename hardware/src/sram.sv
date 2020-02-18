// Copyright 2019 ETH Zurich and University of Bologna.
// All rights reserved.
//
// This code is under development and not yet released to the public.
// Until it is released, the code is under the copyright of ETH Zurich and
// the University of Bologna, and may contain confidential and/or unpublished
// work. Any reuse/redistribution is strictly forbidden without written
// permission from ETH Zurich.

// TODO: Replace behavior with instantiation of cuts.

module sram #(
  parameter int unsigned DATA_WIDTH = 0,   // [bit]
  parameter int unsigned N_WORDS    = 0,
  // Dependent parameters, do not override!
  parameter int unsigned STRB_WIDTH = DATA_WIDTH/8,
  parameter type addr_t = logic[$clog2(N_WORDS)-1:0],
  parameter type data_t = logic[DATA_WIDTH-1:0],
  parameter type strb_t = logic[STRB_WIDTH-1:0]
) (
  input  logic  clk_i,
  input  logic  rst_ni,
  input  logic  req_i,
  input  logic  we_i,
  input  addr_t addr_i,
  input  data_t wdata_i,
  input  strb_t be_i,
  output data_t rdata_o
);

localparam NB_CUTS = N_WORDS / 1024 ;


`ifdef SYNTHESIS
  `ifdef TARGET_XILINX
    strb_t we;
    for (genvar p = 0; p < STRB_WIDTH; p++) begin : gen_we
      assign we[p] = we_i & be_i[p];
    end
    xpm_memory_spram #(
      .ADDR_WIDTH_A         ($clog2(N_WORDS)),
      .AUTO_SLEEP_TIME      (0),
      .BYTE_WRITE_WIDTH_A   (8),
      .CASCADE_HEIGHT       (0),
      .ECC_MODE             ("no_ecc"),
      .MEMORY_INIT_FILE     ("none"),
      .MEMORY_INIT_PARAM    (""),
      .MEMORY_OPTIMIZATION  ("true"),
      .MEMORY_PRIMITIVE     ("block"),
      .MEMORY_SIZE          (N_WORDS*DATA_WIDTH),
      .MESSAGE_CONTROL      (0),
      .READ_DATA_WIDTH_A    (DATA_WIDTH),
      .READ_LATENCY_A       (1),
      .READ_RESET_VALUE_A   ("0"),
      .RST_MODE_A           ("SYNC"),
      .SIM_ASSERT_CHK       (1),
      .USE_MEM_INIT         (0),
      .WAKEUP_TIME          ("disable_sleep"),
      .WRITE_DATA_WIDTH_A   (DATA_WIDTH),
      .WRITE_MODE_A         ("read_first")
    ) i_xpm_memory_spram (
      .addra          (addr_i),
      .clka           (clk_i),
      .dbiterra       (),
      .dina           (wdata_i),
      .douta          (rdata_o),
      .ena            (req_i),
      .injectdbiterra (1'b0),
      .injectsbiterra (1'b0),
      .regcea         (1'b1),
      .rsta           (~rst_ni),
      .sbiterra       (),
      .sleep          (1'b0),
      .wea            (we)
    );
  `else

  logic [31: 0] BE_BW;
  logic [31: 0] Q_int;
  logic [ 4: 0] CEN_int;
  logic [ 3: 0] muxsel;


  assign CEN_int[0] = ~req_i |  addr_i[12] |  addr_i[11] |  addr_i[10];
  assign CEN_int[1] = ~req_i |  addr_i[12] |  addr_i[11] | ~addr_i[10];
  assign CEN_int[2] = ~req_i |  addr_i[12] | ~addr_i[11] |  addr_i[10];
  assign CEN_int[3] = ~req_i |  addr_i[12] | ~addr_i[11] | ~addr_i[10];
  assign CEN_int[4] = ~req_i | ~addr_i[12] |  addr_i[11] |  addr_i[10];

  assign BE_BW      = { {8{be_i[3]}}, {8{be_i[2]}}, {8{be_i[1]}}, {8{be_i[0]}} };

  assign rdata_o = Q_int[muxsel];


  always_ff @(posedge CLK or negedge RSTN)
  begin
    if(~rst_ni)
    begin
      muxsel <= '0;
    end
    else
    begin
      if(~req_i == 1'b0)
        muxsel <= addr_i[12:10];
    end
  end

  generate
    for (genvar i=0; i<NB_CUTS; i++) begin : CUT 

  IN22FDX_R1PH_NFHN_W01024B032M04C256 i_tcdm_bank
  (
    .CLK          ( clk_i            ),
    .CEN          ( CEN_int[i]       ),
    .RDWEN        ( we_i             ),
    .AW           ( addr_i[9:2]      ),
    .AC           ( addr_i[1:0]      ),
    .D            ( wdata_i          ),
    .BW           ( BE_BW            ),
    .Q            ( Q_int[i]         ),   //rdata_o 
    .T_LOGIC      ( 1'b0             ),
    .MA_SAWL      ( '0               ),
    .MA_WL        ( '0               ),
    .MA_WRAS      ( '0               ),
    .MA_WRASD     ( '0               ),
    .OBSV_CTL     (                  )
  );

    end
  endgenerate

  `endif

`else // behavioral
  data_t mem [N_WORDS-1:0];
  always @(posedge clk_i) begin
    if (req_i) begin
      if (we_i) begin
        for (int unsigned i = 0; i < STRB_WIDTH; i++) begin
          if (be_i[i]) begin
            mem[addr_i][i*8+:8] <= wdata_i[i*8+:8];
          end
        end
      end else begin
        rdata_o <= mem[addr_i];
      end
    end
  end
`endif

endmodule
