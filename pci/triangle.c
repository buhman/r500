#define WM(offset, value)                                 \
  do {                                                    \
    wreg(rmmio, offset, value);                           \
    mb();                                                 \
    mdelay(1);                                            \
    printk(KERN_INFO "[r500] %s %08x\n", #offset, value); \
  } while (0);

#define RM(offset) \
  do {             \
    printk(KERN_INFO "[r500] %s %08x\n", #offset, rreg(rmmio, offset)); \
  } while (0);


static void triangle(void __iomem * rmmio)
{
  // r300_emit_gpu_flush
  WM(SC_SCISSOR0, 0);
  uint32_t sc_scissor1 = ((1600 - 1) << 13) | ((1600 - 1) << 0);
  WM(SC_SCISSOR1, sc_scissor1);
  WM(RB3D_DSTCACHE_CTLSTAT, 0xa); // DC_FLUSH | DC_FREE
  WM(ZB_ZCACHE_CTLSTAT, 0x3); // ZC_FLUSH | ZC_FREE
  mb();
  mdelay(100);

  // r300_emit_aa_state
  WM(GB_AA_CONFIG, 0);
  WM(RB3D_AARESOLVE_CTL, 0);

  // r300_emit_fb_state
  WM(RB3D_CCTL, 0x4000); // INDEPENDENT_COLORFORMAT_ENABLE
  WM(RB3D_COLOROFFSET0, 0);
  mb();
  uint32_t rb3d_colorpitch0
    = (1600 << 0) // COLORPITCH
    | (1 << 16) // COLORTILE (is macrotiled)
    | (6 << 21) // COLORFORMAT (ARGB8888)
    ;
  WM(RB3D_COLORPITCH0, rb3d_colorpitch0);
  mb();
  WM(ZB_FORMAT, 2); // 24-bit integer, 8 bit stencil
  WM(ZB_DEPTHOFFSET, 0);
  mb();
  uint32_t zb_depthpitch
    = (1600 << 0)
    | (1 << 16) // DEPTHMACROTILE
    | (0b01 << 17) // DEPTHMICROTILE (32 byte cache line is tiled)
    ;
  WM(ZB_DEPTHPITCH, zb_depthpitch);
  mb();

  // r300_emit_hyperz_state
  WM(ZB_BW_CNTL, 0);
  WM(ZB_DEPTHCLEARVALUE, 0);
  WM(SC_HYPERZ_EN, 0x1c); // HZ_ADJ (add or subtract 1/2)
  WM(GB_Z_PEQ_CONFIG, 0);

  // r300_emit_ztop_state
  WM(ZB_ZTOP, 1); // Z is at top of pipe, after the scan unit

  // r300_emit_dsa_state
  WM(FG_ALPHA_FUNC, 0);
  WM(ZB_CNTL, 0);
  WM(ZB_ZSTENCILCNTL, 0);
  WM(ZB_STENCILREFMASK, 0);
  WM(ZB_STENCILREFMASK_BF, 0);
  WM(FG_ALPHA_VALUE, 0);

  // r300_emit_blend_state
  WM(RB3D_ROPCNTL, 0);
  WM(RB3D_BLENDCNTL, 0);
  WM(RB3D_ABLENDCNTL, 0);
  WM(RB3D_COLOR_CHANNEL_MASK, 0xf); // BLUE_MASK | GREEN_MASK | RED_MASK | ALPHA_MASK
  WM(RB3D_DITHER_CTL, 0);

  // r300_emit_blend_color_state
  WM(RB3D_CONSTANT_COLOR_AR, 0);
  WM(RB3D_CONSTANT_COLOR_GB, 0);

  // r300_emit_scissor_state
  WM(SC_CLIP_0_A, 0);
  WM(SC_CLIP_0_B, 0x3ffffff);

  // r300_emit_sample_mask
  WM(SC_SCREENDOOR, 0xffffff);

  // r300_emit_invariant_state
  WM(GB_SELECT, 0);
  WM(FG_FOG_BLEND, 0);
  WM(GA_OFFSET, 0);
  WM(SU_TEX_WRAP, 0);
  WM(SU_DEPTH_SCALE, 0x4b7fffff); // 16777215.0f
  WM(SU_DEPTH_OFFSET, 0);
  WM(SC_EDGERULE, 0x2da49525);
  WM(RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x1010101);
  WM(RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xfefefefe);
  WM(GA_COLOR_CONTROL_PS3, 0);
  WM(SU_TEX_WRAP_PS3, 0);

  // r300_emit_viewport_state
  WM(VAP_VPORT_XSCALE,  0x44160000); //  600.0f
  WM(VAP_VPORT_XOFFSET, 0x44480000); //  800.0f
  WM(VAP_VPORT_YSCALE,  0xc4160000); // -600.0f
  WM(VAP_VPORT_YOFFSET, 0x44160000); //  600.0f
  WM(VAP_VPORT_ZSCALE,  0x3f000000); //  0.5f
  WM(VAP_VPORT_ZOFFSET, 0x3f000000); //  0.5f

  // r300_emit_pvs_flush
  WM(VAP_PVS_STATE_FLUSH_REG, 0);

  // r300_emit_vap_invariant_state
  WM(VAP_PVS_VTX_TIMEOUT_REG, 0xffff);
  WM(VAP_GB_VERT_CLIP_ADJ, 0x3f800000); // 1.0f
  WM(VAP_GB_VERT_DISC_ADJ, 0x3f800000); // 1.0f
  WM(VAP_GB_HORZ_CLIP_ADJ, 0x3f800000); // 1.0f
  WM(VAP_GB_HORZ_DISC_ADJ, 0x3f800000); // 1.0f
  WM(VAP_PSC_SGN_NORM_CNTL, 0xaaaaaaaa); // SGN_NORM_NO_ZERO
  WM(VAP_TEX_TO_COLOR_CNTL, 0);

  // r300_emit_vertex_stream_state
  WM(VAP_PROG_STREAM_CNTL_0, 0x2002); // DATA_TYPE_0(FLOAT_3) | LAST_VEC_0
  /*
    = SWIZZLE_SELECT_X_0(SELECT_X)
    | SWIZZLE_SELECT_Y_0(SELECT_Y)
    | SWIZZLE_SELECT_Z_0(SELECT_Z)
    | SWIZZLE_SELECT_W_0(SELECT_FP_ONE)
    | WRITE_ENA_0(XYZW)
  */
  WM(VAP_PROG_STREAM_CNTL_EXT_0, 0xfa88);

  // r300_emit_vs_state
  WM(VAP_PVS_CODE_CNTL_0, 0);
  WM(VAP_PVS_CODE_CNTL_1, 0);
  WM(VAP_PVS_VECTOR_INDX_REG, 0);
  const uint32_t vertex_program[] = {0xf00203, 0xd10001, 0x1248001, 0x1248001};
  for (int i = 0; i < (sizeof (vertex_program)) / (sizeof (vertex_program[0])); i++) {
    wreg(rmmio, VAP_PVS_VECTOR_DATA_REG_128, vertex_program[i]);
    mb();
  }

  /*
    = PVS_NUM_SLOTS(10)
    | PVS_NUM_CNTLRS(5)
    | PVS_NUM_FPUS(5)
    | VF_MAX_VTX_NUM(12)
    | TCL_STATE_OPTIMIZATION
   */
  WM(VAP_CNTL, 0xb0055a);
  WM(VAP_PVS_FLOW_CNTL_OPC, 0);
  for (int i = 0; i < 16; i++) {
    wreg(rmmio, VAP_PVS_FLOW_CNTL_ADDRS_LW_0 + i * 8, 0);
    wreg(rmmio, VAP_PVS_FLOW_CNTL_ADDRS_UW_0 + i * 8, 0);
    mb();
  }
  for (int i = 0; i < 16; i++) {
    wreg(rmmio, VAP_PVS_FLOW_CNTL_LOOP_INDEX_0 + i * 4, 0);
    mb();
  }

  // r300_emit_clip_state
  WM(VAP_PVS_VECTOR_INDX_REG, 0x600);
  for (int i = 0; i < 128; i++) {
    wreg(rmmio, VAP_PVS_VECTOR_DATA_REG_128, 0);
    mb();
  }

  // r300_emit_rs_block_stat
  WM(VAP_VTX_STATE_CNTL, 0x5555); // Select User Color 0
  WM(VAP_VSM_VTX_ASSM, 0x1);
  WM(VAP_OUTPUT_VTX_FMT_0, 0x1); // output position vector
  WM(VAP_OUTPUT_VTX_FMT_1, 0x4); // TEX_0_COMP_CNT(4)
  WM(GB_ENABLE, 0);
  WM(RS_IP_0, 0x30000000); // Zero components (0,0,0,1)
  WM(RS_COUNT, 0x40080); // IC_COUNT(1) | HIRES_EN
  WM(RS_INST_COUNT, 0);
  WM(RS_INST_0, 0);

  // r300_emit_rs_state
  WM(VAP_CNTL_STATUS, 0);
  WM(VAP_CLIP_CNTL, 0xc000); // PS_UCP_MODE(3)
  WM(GA_POINT_SIZE, 0x60006);
  WM(GA_POINT_MINMAX, 0x60006);
  WM(GA_LINE_CNTL, 0x20006);
  WM(SU_POLY_OFFSET_ENABLE, 0);
  WM(SU_CULL_MODE, 0);
  WM(GA_LINE_STIPPLE_CONFIG, 0);
  WM(GA_LINE_STIPPLE_VALUE, 0);
  WM(GA_POLY_MODE, 0);
  WM(GA_ROUND_MODE, 0x31); // round to nearest | RGB_CLAMP | ALPHA_CLAMP
  WM(SC_CLIP_RULE, 0xffff);
  WM(GA_POINT_S0, 0); // 0.0f
  WM(GA_POINT_T0, 0x3f800000); // 1.0f
  WM(GA_POINT_S1, 0x3f800000); // 1.0f
  WM(GA_POINT_T1, 0); // 0.0f

  // r300_emit_fb_state_pipelined
  //
  WM(US_OUT_FMT_0, 0x1b00); // C4_8 | C0_SEL(Blue) | C1_SEL(Green) | C2_SEL(Red) | C3_SEL(Alpha)
  WM(US_OUT_FMT_1, 0xf); // unused
  WM(US_OUT_FMT_2, 0xf); // unused
  WM(US_OUT_FMT_3, 0xf); // unused
  WM(GB_MSPOS0, 0x66666666);
  WM(GB_MSPOS0, 0x6666666);

  // r500_emit_fs
  WM(US_CONFIG, 0x2); // Legacy behavior for shader model 1
  WM(US_PIXSIZE, 1);
  WM(US_FC_CTRL, 0);
  WM(US_CODE_RANGE, 0);
  WM(US_CODE_OFFSET, 0);
  WM(US_CODE_ADDR, 0);
  WM(GA_US_VECTOR_INDEX, 0);
  const uint32_t fragment_program[] = {0x78005, 0x8020080, 0x8020080, 0x1c9b04d8, 0x1c810003, 0x5};
  for (int i = 0; i < (sizeof (fragment_program)) / (sizeof (fragment_program[0])); i++) {
    wreg(rmmio, GA_US_VECTOR_DATA, fragment_program[i]);
    mb();
  }
  WM(FG_DEPTH_SRC, 0);
  WM(US_W_FMT, 0);

  // r300_emit_vs_constants
  WM(VAP_PVS_CONST_CNTL, 0);
  // r300_emit_texture_cache_inval
  WM(TX_INVALTAGS, 0);
  // r300_emit_textures_state
  WM(TX_ENABLE, 0);

  // r500_emit_index_bias
  WM(VAP_INDEX_OFFSET, 0);
  // r300_emit_draw_init
  WM(GA_COLOR_CONTROL, 0x3aaaa); // gouraud shading | provoking is always last vertex
  WM(VAP_VF_MAX_VTX_INDX, 2);
  WM(VAP_VF_MIN_VTX_INDX, 0);

  // r300_draw_arrays_immediate
  WM(VAP_VTX_SIZE, 3);

  const float vertices[] = {
    0.5, -0.5, 0,
    -0.5, -0.5, 0,
    0, 0.5, 0,
  };

#define _3D_DRAW_IMMD_2 (0x35)
#define NOP (0x10)
#define TYPE_3_PACKET(opcode, count) \
  ((3 << 30) | ((count) << 16) | (opcode << 8))

  printk(KERN_INFO "[r500] draw start\n");

  wreg(rmmio, 0x1000, TYPE_3_PACKET(_3D_DRAW_IMMD_2, (3 * 3)));
  mb();
  mdelay(1);
  wreg(rmmio, 0x1000, 0x30034); // VAP_VF_CNTL
  mb();
  mdelay(1);
  const uint32_t * vertices_u32 = (const uint32_t *)vertices;
  for (int i = 0; i < 9; i++) {
    wreg(rmmio, 0x1000, vertices_u32[i]);
    mb();
    mdelay(1);
  }
  wreg(rmmio, 0x1000, TYPE_3_PACKET(NOP, 4 - 1));
  mb();
  mdelay(1);
  for (int i = 0; i < 4; i++) {
    wreg(rmmio, 0x1000, 0);
    mb();
    mdelay(1);
  }

  printk(KERN_INFO "[r500] draw end\n");

  WM(0x6110, 0xe0000000);
}
