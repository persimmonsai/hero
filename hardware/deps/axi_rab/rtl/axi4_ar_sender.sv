// Copyright 2018 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

module axi4_ar_sender
  #(
    parameter AXI_ADDR_WIDTH = 40,
    parameter AXI_ID_WIDTH   = 4,
    parameter AXI_USER_WIDTH = 4,
    parameter ENABLE_L2TLB   = 0
  )
  (
    input  logic                      axi4_aclk,
    input  logic                      axi4_arstn,

    output logic                      l1_done_o,
    input  logic                      l1_accept_i,
    input  logic                      l1_drop_i,
    input  logic                      l1_save_i,

    output logic                      l2_done_o,
    input  logic                      l2_accept_i,
    input  logic                      l2_drop_i,
    output logic                      l2_sending_o,

    input  logic [AXI_ADDR_WIDTH-1:0] l1_araddr_i,
    input  logic [AXI_ADDR_WIDTH-1:0] l2_araddr_i,

    input  logic   [AXI_ID_WIDTH-1:0] s_axi4_arid,
    input  logic                      s_axi4_arvalid,
    output logic                      s_axi4_arready,
    input  logic                [7:0] s_axi4_arlen,
    input  logic                [2:0] s_axi4_arsize,
    input  logic                [1:0] s_axi4_arburst,
    input  logic                      s_axi4_arlock,
    input  logic                [2:0] s_axi4_arprot,
    input  logic                [3:0] s_axi4_arcache,
    input  logic [AXI_USER_WIDTH-1:0] s_axi4_aruser,

    output logic   [AXI_ID_WIDTH-1:0] m_axi4_arid,
    output logic [AXI_ADDR_WIDTH-1:0] m_axi4_araddr,
    output logic                      m_axi4_arvalid,
    input  logic                      m_axi4_arready,
    output logic                [7:0] m_axi4_arlen,
    output logic                [2:0] m_axi4_arsize,
    output logic                [1:0] m_axi4_arburst,
    output logic                      m_axi4_arlock,
    output logic                [2:0] m_axi4_arprot,
    output logic                [3:0] m_axi4_arcache,
    output logic [AXI_USER_WIDTH-1:0] m_axi4_aruser
  );

  logic l1_save;

  logic l2_sent;
  logic l2_available_q;

  assign l1_save      = l1_save_i & l2_available_q;

  assign l1_done_o    = s_axi4_arvalid & s_axi4_arready ;

  // if 1: accept and forward a transaction translated by L1
  //    2: drop or save request (if L2 slot not occupied already)
  assign m_axi4_arvalid = (s_axi4_arvalid & l1_accept_i) |
                          l2_sending_o;
  assign s_axi4_arready = (m_axi4_arvalid & m_axi4_arready & ~l2_sending_o) |
                          (s_axi4_arvalid & (l1_drop_i | l1_save));

generate
  if (ENABLE_L2TLB == 1) begin
    logic [AXI_USER_WIDTH-1:0] l2_axi4_aruser  ;
    logic                [3:0] l2_axi4_arcache ;
    logic                [3:0] l2_axi4_arregion;
    logic                [3:0] l2_axi4_arqos   ;
    logic                [2:0] l2_axi4_arprot  ;
    logic                      l2_axi4_arlock  ;
    logic                [1:0] l2_axi4_arburst ;
    logic                [2:0] l2_axi4_arsize  ;
    logic                [7:0] l2_axi4_arlen   ;
    logic   [AXI_ID_WIDTH-1:0] l2_axi4_arid    ;

    assign m_axi4_aruser  = l2_sending_o ? l2_axi4_aruser   : s_axi4_aruser;
    assign m_axi4_arcache = l2_sending_o ? l2_axi4_arcache  : s_axi4_arcache;
    assign m_axi4_arprot  = l2_sending_o ? l2_axi4_arprot   : s_axi4_arprot;
    assign m_axi4_arlock  = l2_sending_o ? l2_axi4_arlock   : s_axi4_arlock;
    assign m_axi4_arburst = l2_sending_o ? l2_axi4_arburst  : s_axi4_arburst;
    assign m_axi4_arsize  = l2_sending_o ? l2_axi4_arsize   : s_axi4_arsize;
    assign m_axi4_arlen   = l2_sending_o ? l2_axi4_arlen    : s_axi4_arlen;
    assign m_axi4_araddr  = l2_sending_o ? l2_araddr_i      : l1_araddr_i;
    assign m_axi4_arid    = l2_sending_o ? l2_axi4_arid     : s_axi4_arid;

    // Buffer AXI signals in case of L1 miss
    always @(posedge axi4_aclk or negedge axi4_arstn) begin
      if (axi4_arstn == 1'b0) begin
        l2_axi4_aruser  <=  'b0;
        l2_axi4_arcache <=  'b0;
        l2_axi4_arprot  <=  'b0;
        l2_axi4_arlock  <= 1'b0;
        l2_axi4_arburst <=  'b0;
        l2_axi4_arsize  <=  'b0;
        l2_axi4_arlen   <=  'b0;
        l2_axi4_arid    <=  'b0;
      end else if (l1_save) begin
        l2_axi4_aruser  <= s_axi4_aruser;
        l2_axi4_arcache <= s_axi4_arcache;
        l2_axi4_arprot  <= s_axi4_arprot;
        l2_axi4_arlock  <= s_axi4_arlock;
        l2_axi4_arburst <= s_axi4_arburst;
        l2_axi4_arsize  <= s_axi4_arsize;
        l2_axi4_arlen   <= s_axi4_arlen;
        l2_axi4_arid    <= s_axi4_arid;
      end
    end

    // signal that an l1_save_i can be accepted
    always @(posedge axi4_aclk or negedge axi4_arstn) begin
      if (axi4_arstn == 1'b0) begin
        l2_available_q <= 1'b1;
      end else if (l2_sent | l2_drop_i) begin
        l2_available_q <= 1'b1;
      end else if (l1_save) begin
        l2_available_q <= 1'b0;
      end
    end

    assign l2_sending_o = l2_accept_i & ~l2_available_q;
    assign l2_sent      = l2_sending_o & m_axi4_arvalid & m_axi4_arready;

    // if 1: having sent out a transaction translated by L2
    //    2: drop request (L2 slot is available again)
    assign l2_done_o    = l2_sent | l2_drop_i;

  end else begin // !`ifdef ENABLE_L2TLB
    assign m_axi4_aruser  =  s_axi4_aruser;
    assign m_axi4_arcache =  s_axi4_arcache;
    assign m_axi4_arprot  =  s_axi4_arprot;
    assign m_axi4_arlock  =  s_axi4_arlock;
    assign m_axi4_arburst =  s_axi4_arburst;
    assign m_axi4_arsize  =  s_axi4_arsize;
    assign m_axi4_arlen   =  s_axi4_arlen;
    assign m_axi4_araddr  =  l1_araddr_i;
    assign m_axi4_arid    =  s_axi4_arid;

    assign l2_sending_o   = 1'b0;
    assign l2_available_q = 1'b0;
    assign l2_done_o      = 1'b0;
  end // else: !if(ENABLE_L2TLB == 1)
endgenerate

endmodule
