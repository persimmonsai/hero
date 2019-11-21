module hero_zcu102 (
  input           clk_i,
  input           rst_ni,
  output [  5:0]  mst_aw_id_o,
  output [ 48:0]  mst_aw_addr_o,
  output [  7:0]  mst_aw_len_o,
  output [  2:0]  mst_aw_size_o,
  output [  1:0]  mst_aw_burst_o,
  output          mst_aw_lock_o,
  output [  3:0]  mst_aw_cache_o,
  output [  2:0]  mst_aw_prot_o,
  output [  3:0]  mst_aw_qos_o,
  output          mst_aw_user_o,
  output          mst_aw_valid_o,
  input           mst_aw_ready_i,
  output [127:0]  mst_w_data_o,
  output [ 15:0]  mst_w_strb_o,
  output          mst_w_last_o,
  output          mst_w_valid_o,
  input           mst_w_ready_i,
  input  [  5:0]  mst_b_id_i,
  input  [  1:0]  mst_b_resp_i,
  input           mst_b_valid_i,
  output          mst_b_ready_o,
  output [  5:0]  mst_ar_id_o,
  output [ 48:0]  mst_ar_addr_o,
  output [  7:0]  mst_ar_len_o,
  output [  2:0]  mst_ar_size_o,
  output [  1:0]  mst_ar_burst_o,
  output          mst_ar_lock_o,
  output [  3:0]  mst_ar_cache_o,
  output [  2:0]  mst_ar_prot_o,
  output [  3:0]  mst_ar_qos_o,
  output          mst_ar_user_o,
  output          mst_ar_valid_o,
  input           mst_ar_ready_i,
  input  [  5:0]  mst_r_id_i,
  input  [127:0]  mst_r_data_i,
  input  [  1:0]  mst_r_resp_i,
  input           mst_r_last_i,
  input           mst_r_valid_i,
  output          mst_r_ready_o,
  input  [ 16:0]  slv_aw_id_i,
  input  [ 39:0]  slv_aw_addr_i,
  input  [  7:0]  slv_aw_len_i,
  input  [  2:0]  slv_aw_size_i,
  input  [  1:0]  slv_aw_burst_i,
  input           slv_aw_lock_i,
  input  [  3:0]  slv_aw_cache_i,
  input  [  2:0]  slv_aw_prot_i,
  input  [  3:0]  slv_aw_qos_i,
  input  [ 15:0]  slv_aw_user_i,
  input           slv_aw_valid_i,
  output          slv_aw_ready_o,
  input  [127:0]  slv_w_data_i,
  input  [ 15:0]  slv_w_strb_i,
  input           slv_w_last_i,
  input           slv_w_valid_i,
  output          slv_w_ready_o,
  output [ 16:0]  slv_b_id_o,
  output [  1:0]  slv_b_resp_o,
  output          slv_b_valid_o,
  input           slv_b_ready_i,
  input  [ 16:0]  slv_ar_id_i,
  input  [ 39:0]  slv_ar_addr_i,
  input  [  7:0]  slv_ar_len_i,
  input  [  2:0]  slv_ar_size_i,
  input  [  1:0]  slv_ar_burst_i,
  input           slv_ar_lock_i,
  input  [  3:0]  slv_ar_cache_i,
  input  [  2:0]  slv_ar_prot_i,
  input  [  3:0]  slv_ar_qos_i,
  input  [ 15:0]  slv_ar_user_i,
  input           slv_ar_valid_i,
  output          slv_ar_ready_o,
  output [ 16:0]  slv_r_id_o,
  output [127:0]  slv_r_data_o,
  output [  1:0]  slv_r_resp_o,
  output          slv_r_last_o,
  output          slv_r_valid_o,
  input           slv_r_ready_i,
  input  [ 31:0]  rab_conf_aw_addr_i,
  input  [  2:0]  rab_conf_aw_prot_i,
  input           rab_conf_aw_valid_i,
  output          rab_conf_aw_ready_o,
  input  [ 31:0]  rab_conf_w_data_i,
  input  [  3:0]  rab_conf_w_strb_i,
  input           rab_conf_w_valid_i,
  output          rab_conf_w_ready_o,
  output [  1:0]  rab_conf_b_resp_o,
  output          rab_conf_b_valid_o,
  input           rab_conf_b_ready_i,
  input  [ 31:0]  rab_conf_ar_addr_i,
  input  [  2:0]  rab_conf_ar_prot_i,
  input           rab_conf_ar_valid_i,
  output          rab_conf_ar_ready_o,
  output [ 31:0]  rab_conf_r_data_o,
  output [  1:0]  rab_conf_r_resp_o,
  output          rab_conf_r_valid_o,
  input           rab_conf_r_ready_i,
  input           cl_fetch_en_i,
  output          cl_busy_o,
  output          cl_eoc_o,
  output          rab_from_pulp_miss_irq_o,
  output          rab_from_pulp_multi_irq_o,
  output          rab_from_pulp_prot_irq_o,
  output          rab_from_host_miss_irq_o,
  output          rab_from_host_multi_irq_o,
  output          rab_from_host_prot_irq_o,
  output          rab_miss_fifo_full_irq_o
);
  wire [  6:0] slv_aw_id,     slv_b_id,                   slv_ar_id,        slv_r_id,
               mst_aw_id,     mst_b_id,                   mst_ar_id,        mst_r_id;
  wire [ 63:0] slv_aw_addr,                               slv_ar_addr,
               mst_aw_addr,                               mst_ar_addr,
               mst_aw_addr_idr,                           mst_ar_addr_idr;
  wire [  7:0] slv_aw_len,                                slv_ar_len,
               mst_aw_len,                                mst_ar_len;
  wire [  2:0] slv_aw_size,                               slv_ar_size,
               mst_aw_size,                               mst_ar_size;
  wire [  1:0] slv_aw_burst,                              slv_ar_burst,
               mst_aw_burst,                              mst_ar_burst;
  wire         slv_aw_lock,                               slv_ar_lock,
               mst_aw_lock,                               mst_ar_lock;
  wire [  3:0] slv_aw_cache,                              slv_ar_cache,
               mst_aw_cache,                              mst_ar_cache;
  wire [  2:0] slv_aw_prot,                               slv_ar_prot,
               mst_aw_prot,                               mst_ar_prot;
  wire [  3:0] slv_aw_qos,                                slv_ar_qos,
               mst_aw_qos,                                mst_ar_qos;
  wire [  3:0] slv_aw_region,                             slv_ar_region,
               mst_aw_region,                             mst_ar_region;
  wire [  5:0] slv_aw_atop,
               mst_aw_atop;
  wire [  3:0] slv_aw_user,   slv_b_user,   slv_w_user,   slv_ar_user,      slv_r_user,
               mst_aw_user,   mst_b_user,   mst_w_user,   mst_ar_user,      mst_r_user,
               mst_aw_user_idr,                           mst_ar_user_idr;
  wire         slv_aw_valid,  slv_b_valid,  slv_w_valid,  slv_ar_valid,     slv_r_valid,
               mst_aw_valid,  mst_b_valid,  mst_w_valid,  mst_ar_valid,     mst_r_valid,
               slv_aw_ready,  slv_b_ready,  slv_w_ready,  slv_ar_ready,     slv_r_ready,
               mst_aw_ready,  mst_b_ready,  mst_w_ready,  mst_ar_ready,     mst_r_ready;
  wire [127:0]                              slv_w_data,                     slv_r_data,
                                            mst_w_data,                     mst_r_data;
  wire [ 15:0]                              slv_w_strb,
                                            mst_w_strb;
  wire [  1:0]                slv_b_resp,                                   slv_r_resp,
                              mst_b_resp,                                   mst_r_resp;
  wire                                      slv_w_last,                     slv_r_last,
                                            mst_w_last,                     mst_r_last;
  hero_ooc #(
    .N_CLUSTERS     (1),
    .AXI_DW         (128),
    .L2_N_AXI_PORTS (1)
  ) i_bound (
    .clk_i            (clk_i),
    .rst_ni           (rst_ni),
    .mst_aw_id_o      (mst_aw_id),
    .mst_aw_addr_o    (mst_aw_addr),
    .mst_aw_len_o     (mst_aw_len),
    .mst_aw_size_o    (mst_aw_size),
    .mst_aw_burst_o   (mst_aw_burst),
    .mst_aw_lock_o    (mst_aw_lock),
    .mst_aw_cache_o   (mst_aw_cache),
    .mst_aw_prot_o    (mst_aw_prot),
    .mst_aw_qos_o     (mst_aw_qos),
    .mst_aw_region_o  (mst_aw_region),
    .mst_aw_atop_o    (mst_aw_atop),
    .mst_aw_user_o    (mst_aw_user),
    .mst_aw_valid_o   (mst_aw_valid),
    .mst_aw_ready_i   (mst_aw_ready),
    .mst_w_data_o     (mst_w_data),
    .mst_w_strb_o     (mst_w_strb),
    .mst_w_last_o     (mst_w_last),
    .mst_w_user_o     (mst_w_user),
    .mst_w_valid_o    (mst_w_valid),
    .mst_w_ready_i    (mst_w_ready),
    .mst_b_id_i       (mst_b_id),
    .mst_b_resp_i     (mst_b_resp),
    .mst_b_user_i     (mst_b_user),
    .mst_b_valid_i    (mst_b_valid),
    .mst_b_ready_o    (mst_b_ready),
    .mst_ar_id_o      (mst_ar_id),
    .mst_ar_addr_o    (mst_ar_addr),
    .mst_ar_len_o     (mst_ar_len),
    .mst_ar_size_o    (mst_ar_size),
    .mst_ar_burst_o   (mst_ar_burst),
    .mst_ar_lock_o    (mst_ar_lock),
    .mst_ar_cache_o   (mst_ar_cache),
    .mst_ar_prot_o    (mst_ar_prot),
    .mst_ar_qos_o     (mst_ar_qos),
    .mst_ar_region_o  (mst_ar_region),
    .mst_ar_user_o    (mst_ar_user),
    .mst_ar_valid_o   (mst_ar_valid),
    .mst_ar_ready_i   (mst_ar_ready),
    .mst_r_id_i       (mst_r_id),
    .mst_r_data_i     (mst_r_data),
    .mst_r_resp_i     (mst_r_resp),
    .mst_r_last_i     (mst_r_last),
    .mst_r_user_i     (mst_r_user),
    .mst_r_valid_i    (mst_r_valid),
    .mst_r_ready_o    (mst_r_ready),
    .slv_aw_id_i      (slv_aw_id),
    .slv_aw_addr_i    (slv_aw_addr),
    .slv_aw_len_i     (slv_aw_len),
    .slv_aw_size_i    (slv_aw_size),
    .slv_aw_burst_i   (slv_aw_burst),
    .slv_aw_lock_i    (slv_aw_lock),
    .slv_aw_cache_i   (slv_aw_cache),
    .slv_aw_prot_i    (slv_aw_prot),
    .slv_aw_qos_i     (slv_aw_qos),
    .slv_aw_region_i  (slv_aw_region),
    .slv_aw_atop_i    (slv_aw_atop),
    .slv_aw_user_i    (slv_aw_user),
    .slv_aw_valid_i   (slv_aw_valid),
    .slv_aw_ready_o   (slv_aw_ready),
    .slv_w_data_i     (slv_w_data),
    .slv_w_strb_i     (slv_w_strb),
    .slv_w_last_i     (slv_w_last),
    .slv_w_user_i     (slv_w_user),
    .slv_w_valid_i    (slv_w_valid),
    .slv_w_ready_o    (slv_w_ready),
    .slv_b_id_o       (slv_b_id),
    .slv_b_resp_o     (slv_b_resp),
    .slv_b_user_o     (slv_b_user),
    .slv_b_valid_o    (slv_b_valid),
    .slv_b_ready_i    (slv_b_ready),
    .slv_ar_id_i      (slv_ar_id),
    .slv_ar_addr_i    (slv_ar_addr),
    .slv_ar_len_i     (slv_ar_len),
    .slv_ar_size_i    (slv_ar_size),
    .slv_ar_burst_i   (slv_ar_burst),
    .slv_ar_lock_i    (slv_ar_lock),
    .slv_ar_cache_i   (slv_ar_cache),
    .slv_ar_prot_i    (slv_ar_prot),
    .slv_ar_qos_i     (slv_ar_qos),
    .slv_ar_region_i  (slv_ar_region),
    .slv_ar_user_i    (slv_ar_user),
    .slv_ar_valid_i   (slv_ar_valid),
    .slv_ar_ready_o   (slv_ar_ready),
    .slv_r_id_o       (slv_r_id),
    .slv_r_data_o     (slv_r_data),
    .slv_r_resp_o     (slv_r_resp),
    .slv_r_last_o     (slv_r_last),
    .slv_r_user_o     (slv_r_user),
    .slv_r_valid_o    (slv_r_valid),
    .slv_r_ready_i    (slv_r_ready),
    .rab_conf_aw_id_i     (1'b0),
    .rab_conf_aw_addr_i   (rab_conf_aw_addr_i[31:0]),
    .rab_conf_aw_size_i   (3'b010),
    .rab_conf_aw_prot_i   (rab_conf_aw_prot_i),
    .rab_conf_aw_user_i   (1'b0),
    .rab_conf_aw_valid_i  (rab_conf_aw_valid_i),
    .rab_conf_aw_ready_o  (rab_conf_aw_ready_o),
    .rab_conf_w_data_i    (rab_conf_w_data_i),
    .rab_conf_w_strb_i    (rab_conf_w_strb_i),
    .rab_conf_w_user_i    (1'b0),
    .rab_conf_w_valid_i   (rab_conf_w_valid_i),
    .rab_conf_w_ready_o   (rab_conf_w_ready_o),
    .rab_conf_b_id_o      (/* unused */),
    .rab_conf_b_resp_o    (rab_conf_b_resp_o),
    .rab_conf_b_user_o    (/* unused */),
    .rab_conf_b_valid_o   (rab_conf_b_valid_o),
    .rab_conf_b_ready_i   (rab_conf_b_ready_i),
    .rab_conf_ar_id_i     (1'b0),
    .rab_conf_ar_addr_i   (rab_conf_ar_addr_i[31:0]),
    .rab_conf_ar_size_i   (3'b010),
    .rab_conf_ar_prot_i   (rab_conf_ar_prot_i),
    .rab_conf_ar_user_i   (1'b0),
    .rab_conf_ar_valid_i  (rab_conf_ar_valid_i),
    .rab_conf_ar_ready_o  (rab_conf_ar_ready_o),
    .rab_conf_r_id_o      (/* unused */),
    .rab_conf_r_data_o    (rab_conf_r_data_o),
    .rab_conf_r_resp_o    (rab_conf_r_resp_o),
    .rab_conf_r_user_o    (/* unused */),
    .rab_conf_r_valid_o   (rab_conf_r_valid_o),
    .rab_conf_r_ready_i   (rab_conf_r_ready_i),
    .cl_fetch_en_i  (cl_fetch_en_i),
    .cl_eoc_o       (cl_eoc_o),
    .cl_busy_o      (cl_busy_o),
    .rab_from_pulp_miss_irq_o   (rab_from_pulp_miss_irq_o),
    .rab_from_pulp_multi_irq_o  (rab_from_pulp_multi_irq_o),
    .rab_from_pulp_prot_irq_o   (rab_from_pulp_prot_irq_o),
    .rab_from_host_miss_irq_o   (rab_from_host_miss_irq_o),
    .rab_from_host_multi_irq_o  (rab_from_host_multi_irq_o),
    .rab_from_host_prot_irq_o   (rab_from_host_prot_irq_o),
    .rab_miss_fifo_full_irq_o   (rab_miss_fifo_full_irq_o)
  );
  axi_id_resize_ports #(
    .ADDR_WIDTH   (64),
    .DATA_WIDTH   (128),
    .USER_WIDTH   (4),
    .ID_WIDTH_IN  (7),
    .ID_WIDTH_OUT (6),
    .TABLE_SIZE   (16)
  ) i_id_resize_mst (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .in_aw_id_i     (mst_aw_id),
    .in_aw_addr_i   (mst_aw_addr),
    .in_aw_len_i    (mst_aw_len),
    .in_aw_size_i   (mst_aw_size),
    .in_aw_burst_i  (mst_aw_burst),
    .in_aw_lock_i   (mst_aw_lock),
    .in_aw_cache_i  (mst_aw_cache),
    .in_aw_prot_i   (mst_aw_prot),
    .in_aw_qos_i    (mst_aw_qos),
    .in_aw_region_i (mst_aw_region),
    .in_aw_atop_i   (mst_aw_atop),
    .in_aw_user_i   (mst_aw_user),
    .in_aw_valid_i  (mst_aw_valid),
    .in_aw_ready_o  (mst_aw_ready),
    .in_w_data_i    (mst_w_data),
    .in_w_strb_i    (mst_w_strb),
    .in_w_last_i    (mst_w_last),
    .in_w_user_i    (mst_w_user),
    .in_w_valid_i   (mst_w_valid),
    .in_w_ready_o   (mst_w_ready),
    .in_b_id_o      (mst_b_id),
    .in_b_resp_o    (mst_b_resp),
    .in_b_user_o    (mst_b_user),
    .in_b_valid_o   (mst_b_valid),
    .in_b_ready_i   (mst_b_ready),
    .in_ar_id_i     (mst_ar_id),
    .in_ar_addr_i   (mst_ar_addr),
    .in_ar_len_i    (mst_ar_len),
    .in_ar_size_i   (mst_ar_size),
    .in_ar_burst_i  (mst_ar_burst),
    .in_ar_lock_i   (mst_ar_lock),
    .in_ar_cache_i  (mst_ar_cache),
    .in_ar_prot_i   (mst_ar_prot),
    .in_ar_qos_i    (mst_ar_qos),
    .in_ar_region_i (mst_ar_region),
    .in_ar_user_i   (mst_ar_user),
    .in_ar_valid_i  (mst_ar_valid),
    .in_ar_ready_o  (mst_ar_ready),
    .in_r_id_o      (mst_r_id),
    .in_r_data_o    (mst_r_data),
    .in_r_resp_o    (mst_r_resp),
    .in_r_last_o    (mst_r_last),
    .in_r_user_o    (mst_r_user),
    .in_r_valid_o   (mst_r_valid),
    .in_r_ready_i   (mst_r_ready),
    .out_aw_id_o      (mst_aw_id_o),
    .out_aw_addr_o    (mst_aw_addr_idr),
    .out_aw_len_o     (mst_aw_len_o),
    .out_aw_size_o    (mst_aw_size_o),
    .out_aw_burst_o   (mst_aw_burst_o),
    .out_aw_lock_o    (mst_aw_lock_o),
    .out_aw_cache_o   (mst_aw_cache_o),
    .out_aw_prot_o    (mst_aw_prot_o),
    .out_aw_qos_o     (mst_aw_qos_o),
    .out_aw_region_o  (/* unused */),
    .out_aw_atop_o    (/* unused */),
    .out_aw_user_o    (mst_aw_user_idr),
    .out_aw_valid_o   (mst_aw_valid_o),
    .out_aw_ready_i   (mst_aw_ready_i),
    .out_w_data_o     (mst_w_data_o),
    .out_w_strb_o     (mst_w_strb_o),
    .out_w_last_o     (mst_w_last_o),
    .out_w_user_o      (/* unused */),
    .out_w_valid_o    (mst_w_valid_o),
    .out_w_ready_i    (mst_w_ready_i),
    .out_b_id_i       (mst_b_id_i),
    .out_b_resp_i     (mst_b_resp_i),
    .out_b_user_i     ({4{1'b0}}),
    .out_b_valid_i    (mst_b_valid_i),
    .out_b_ready_o    (mst_b_ready_o),
    .out_ar_id_o      (mst_ar_id_o),
    .out_ar_addr_o    (mst_ar_addr_idr),
    .out_ar_len_o     (mst_ar_len_o),
    .out_ar_size_o    (mst_ar_size_o),
    .out_ar_burst_o   (mst_ar_burst_o),
    .out_ar_lock_o    (mst_ar_lock_o),
    .out_ar_cache_o   (mst_ar_cache_o),
    .out_ar_prot_o    (mst_ar_prot_o),
    .out_ar_qos_o     (mst_ar_qos_o),
    .out_ar_region_o  (/* unused */),
    .out_ar_user_o    (mst_ar_user_idr),
    .out_ar_valid_o   (mst_ar_valid_o),
    .out_ar_ready_i   (mst_ar_ready_i),
    .out_r_id_i       (mst_r_id_i),
    .out_r_data_i     (mst_r_data_i),
    .out_r_resp_i     (mst_r_resp_i),
    .out_r_last_i     (mst_r_last_i),
    .out_r_user_i     ({4{1'b0}}),
    .out_r_valid_i    (mst_r_valid_i),
    .out_r_ready_o    (mst_r_ready_o)
  );
  assign mst_aw_addr_o = mst_aw_addr_idr[48:0];
  assign mst_ar_addr_o = mst_ar_addr_idr[48:0];
  assign mst_aw_user_o = mst_aw_user_idr[0];
  assign mst_ar_user_o = mst_ar_user_idr[0];
  axi_id_resize_ports #(
    .ADDR_WIDTH   (64),
    .DATA_WIDTH   (128),
    .USER_WIDTH   (4),
    .ID_WIDTH_IN  (17),
    .ID_WIDTH_OUT (7),
    .TABLE_SIZE   (4)
  ) i_id_resize_slv (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .in_aw_id_i     (slv_aw_id_i),
    .in_aw_addr_i   ({{24{1'b0}}, slv_aw_addr_i}),
    .in_aw_len_i    (slv_aw_len_i),
    .in_aw_size_i   (slv_aw_size_i),
    .in_aw_burst_i  (slv_aw_burst_i),
    .in_aw_lock_i   (slv_aw_lock_i),
    .in_aw_cache_i  (slv_aw_cache_i),
    .in_aw_prot_i   (slv_aw_prot_i),
    .in_aw_qos_i    (slv_aw_qos_i),
    .in_aw_region_i ({4{1'b0}}),
    .in_aw_atop_i   ({6{1'b0}}),
    .in_aw_user_i   (slv_aw_user_i[3:0]),
    .in_aw_valid_i  (slv_aw_valid_i),
    .in_aw_ready_o  (slv_aw_ready_o),
    .in_w_data_i    (slv_w_data_i),
    .in_w_strb_i    (slv_w_strb_i),
    .in_w_last_i    (slv_w_last_i),
    .in_w_user_i    ({4{1'b0}}),
    .in_w_valid_i   (slv_w_valid_i),
    .in_w_ready_o   (slv_w_ready_o),
    .in_b_id_o      (slv_b_id_o),
    .in_b_resp_o    (slv_b_resp_o),
    .in_b_user_o    (/* unused */),
    .in_b_valid_o   (slv_b_valid_o),
    .in_b_ready_i   (slv_b_ready_i),
    .in_ar_id_i     (slv_ar_id_i),
    .in_ar_addr_i   ({{24{1'b0}}, slv_ar_addr_i}),
    .in_ar_len_i    (slv_ar_len_i),
    .in_ar_size_i   (slv_ar_size_i),
    .in_ar_burst_i  (slv_ar_burst_i),
    .in_ar_lock_i   (slv_ar_lock_i),
    .in_ar_cache_i  (slv_ar_cache_i),
    .in_ar_prot_i   (slv_ar_prot_i),
    .in_ar_qos_i    (slv_ar_qos_i),
    .in_ar_region_i ({4{1'b0}}),
    .in_ar_user_i   (slv_ar_user_i[3:0]),
    .in_ar_valid_i  (slv_ar_valid_i),
    .in_ar_ready_o  (slv_ar_ready_o),
    .in_r_id_o      (slv_r_id_o),
    .in_r_data_o    (slv_r_data_o),
    .in_r_resp_o    (slv_r_resp_o),
    .in_r_last_o    (slv_r_last_o),
    .in_r_user_o    (/* unused */),
    .in_r_valid_o   (slv_r_valid_o),
    .in_r_ready_i   (slv_r_ready_i),
    .out_aw_id_o      (slv_aw_id),
    .out_aw_addr_o    (slv_aw_addr),
    .out_aw_len_o     (slv_aw_len),
    .out_aw_size_o    (slv_aw_size),
    .out_aw_burst_o   (slv_aw_burst),
    .out_aw_lock_o    (slv_aw_lock),
    .out_aw_cache_o   (slv_aw_cache),
    .out_aw_prot_o    (slv_aw_prot),
    .out_aw_qos_o     (slv_aw_qos),
    .out_aw_region_o  (slv_aw_region),
    .out_aw_atop_o    (slv_aw_atop),
    .out_aw_user_o    (slv_aw_user),
    .out_aw_valid_o   (slv_aw_valid),
    .out_aw_ready_i   (slv_aw_ready),
    .out_w_data_o     (slv_w_data),
    .out_w_strb_o     (slv_w_strb),
    .out_w_last_o     (slv_w_last),
    .out_w_user_o     (slv_w_user),
    .out_w_valid_o    (slv_w_valid),
    .out_w_ready_i    (slv_w_ready),
    .out_b_id_i       (slv_b_id),
    .out_b_resp_i     (slv_b_resp),
    .out_b_user_i     (slv_b_user),
    .out_b_valid_i    (slv_b_valid),
    .out_b_ready_o    (slv_b_ready),
    .out_ar_id_o      (slv_ar_id),
    .out_ar_addr_o    (slv_ar_addr),
    .out_ar_len_o     (slv_ar_len),
    .out_ar_size_o    (slv_ar_size),
    .out_ar_burst_o   (slv_ar_burst),
    .out_ar_lock_o    (slv_ar_lock),
    .out_ar_cache_o   (slv_ar_cache),
    .out_ar_prot_o    (slv_ar_prot),
    .out_ar_qos_o     (slv_ar_qos),
    .out_ar_region_o  (slv_ar_region),
    .out_ar_user_o    (slv_ar_user),
    .out_ar_valid_o   (slv_ar_valid),
    .out_ar_ready_i   (slv_ar_ready),
    .out_r_id_i       (slv_r_id),
    .out_r_data_i     (slv_r_data),
    .out_r_resp_i     (slv_r_resp),
    .out_r_last_i     (slv_r_last),
    .out_r_user_i     (slv_r_user),
    .out_r_valid_i    (slv_r_valid),
    .out_r_ready_o    (slv_r_ready)
  );
endmodule
