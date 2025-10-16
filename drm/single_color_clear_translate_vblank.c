#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>
#include <libdrm/radeon_drm.h>

#include "3d_registers.h"
#include "3d_registers_undocumented.h"
#include "3d_registers_bits.h"
#include "command_processor.h"

static inline uint32_t rreg(void * rmmio, uint32_t offset)
{
  uint32_t value = *((volatile uint32_t *)(((uintptr_t)rmmio) + offset));
  asm volatile ("" ::: "memory");
  return value;
}

static inline void wreg(void * rmmio, uint32_t offset, uint32_t value)
{
  *((volatile uint32_t *)(((uintptr_t)rmmio) + offset)) = value;
  asm volatile ("" ::: "memory");
}

union u32_f32 {
  uint32_t u32;
  float f32;
};

static union u32_f32 ib[16384];

float translate_x = 0;

int indirect_buffer_setup(int ix)
{
  T0V(RB3D_DSTCACHE_CTLSTAT
      , RB3D_DSTCACHE_CTLSTAT__DC_FLUSH(0x2) // Flush dirty 3D data
      | RB3D_DSTCACHE_CTLSTAT__DC_FREE(0x2)  // Free 3D tags
      );

  T0V(ZB_ZCACHE_CTLSTAT
      , ZB_ZCACHE_CTLSTAT__ZC_FLUSH(1)
      | ZB_ZCACHE_CTLSTAT__ZC_FREE(1)
      );

  T0V(WAIT_UNTIL, 0x00020000);

  T0V(GB_AA_CONFIG, 0x00000000);

  T0V(RB3D_AARESOLVE_CTL, 0x00000000);

  T0V(RB3D_CCTL
      , RB3D_CCTL__INDEPENDENT_COLORFORMAT_ENABLE(1)
      );

  T0V(ZB_BW_CNTL, 0x00000000);
  T0V(ZB_DEPTHCLEARVALUE, 0x00000000);
  T0V(SC_HYPERZ_EN, 0x00000000);
  T0V(GB_Z_PEQ_CONFIG, 0x00000000);
  T0V(ZB_ZTOP
      , ZB_ZTOP__ZTOP(1)
      );
  T0V(FG_ALPHA_FUNC, 0x00000000);
  T0V(ZB_CNTL, 0x00000000);
  T0V(ZB_ZSTENCILCNTL, 0x00000000);
  T0V(ZB_STENCILREFMASK, 0x00000000);
  T0V(ZB_STENCILREFMASK_BF, 0x00000000);

  T0V(FG_ALPHA_VALUE, 0x00000000);
  T0V(RB3D_ROPCNTL, 0x00000000);
  T0V(RB3D_BLENDCNTL, 0x00000000);
  T0V(RB3D_ABLENDCNTL, 0x00000000);
  T0V(RB3D_COLOR_CHANNEL_MASK
      , RB3D_COLOR_CHANNEL_MASK__BLUE_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__GREEN_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__RED_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__ALPHA_MASK(1)
      );
  T0V(RB3D_DITHER_CTL, 0x00000000);
  T0V(RB3D_CONSTANT_COLOR_AR, 0x00000000);
  T0V(RB3D_CONSTANT_COLOR_GB, 0x00000000);

  T0V(SC_CLIP_0_A, 0x00000000);
  T0V(SC_CLIP_0_B, 0xffffffff);
  T0V(SC_SCREENDOOR, 0x00ffffff);

  T0V(GB_SELECT, 0x00000000);
  T0V(FG_FOG_BLEND, 0x00000000);
  T0V(GA_OFFSET, 0x00000000);
  T0V(SU_TEX_WRAP, 0x00000000);
  T0Vf(SU_DEPTH_SCALE, 16777215.0f);
  T0V(SU_DEPTH_OFFSET, 0x00000000);
  T0V(SC_EDGERULE
      , SC_EDGERULE__ER_TRI(5)      // L-in,R-out,HT-in,HB-in
      | SC_EDGERULE__ER_POINT(9)    // L-out,R-in,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_LR(5)  // L-in,R-out,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_RL(9)  // L-out,R-in,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_TB(26) // T-in,B-out,VL-out,VR-in
      | SC_EDGERULE__ER_LINE_BT(22) // T-out,B-in,VL-out,VR-in
      );
  T0V(RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD
      , RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD__BLUE(1)
      | RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD__GREEN(1)
      | RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD__RED(1)
      | RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD__ALPHA(1)
      );
  T0V(RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD
      , RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD__BLUE(254)
      | RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD__GREEN(254)
      | RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD__RED(254)
      | RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD__ALPHA(254)
      );
  T0V(GA_COLOR_CONTROL_PS3, 0x00000000);
  T0V(SU_TEX_WRAP_PS3, 0x00000000);
  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);
  T0V(VAP_PVS_VTX_TIMEOUT_REG
      , VAP_PVS_VTX_TIMEOUT_REG__CLK_COUNT(0xffff)
      );
  T0Vf(VAP_GB_VERT_CLIP_ADJ, 1.0f);
  T0Vf(VAP_GB_VERT_DISC_ADJ, 1.0f);
  T0Vf(VAP_GB_HORZ_CLIP_ADJ, 1.0f);
  T0Vf(VAP_GB_HORZ_DISC_ADJ, 1.0f);
  T0V(VAP_PSC_SGN_NORM_CNTL
      , VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_0(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_1(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_2(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_3(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_4(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_5(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_6(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_7(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_8(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_9(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_10(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_11(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_12(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_13(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_14(2)
      | VAP_PSC_SGN_NORM_CNTL__SGN_NORM_METHOD_15(2)
      );
  T0V(VAP_TEX_TO_COLOR_CNTL, 0x00000000);

  T0V(VAP_CNTL
      , VAP_CNTL__PVS_NUM_SLOTS(10)
      | VAP_CNTL__PVS_NUM_CNTLRS(5)
      | VAP_CNTL__PVS_NUM_FPUS(5)
      | VAP_CNTL__VAP_NO_RENDER(0)
      | VAP_CNTL__VF_MAX_VTX_NUM(12)
      | VAP_CNTL__DX_CLIP_SPACE_DEF(0)
      | VAP_CNTL__TCL_STATE_OPTIMIZATION(1)
      );
  T0V(VAP_PVS_FLOW_CNTL_OPC, 0x00000000);

  T0(VAP_PVS_FLOW_CNTL_ADDRS_LW_0, 31);
  for (int i = 0; i < 32; i++)
    ib[ix++].u32 = 0x00000000;

  T0(VAP_PVS_FLOW_CNTL_LOOP_INDEX_0, 15);
  for (int i = 0; i < 16; i++)
    ib[ix++].u32 = 0x00000000;

  T0V(VAP_PVS_VECTOR_INDX_REG, 0x00000600);
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, 23);
  for (int i = 0; i < 24; i++)
    ib[ix++].u32 = 0x00000000;

  T0V(VAP_VTX_STATE_CNTL
      , VAP_VTX_STATE_CNTL__COLOR_0_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_1_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_2_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_3_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_4_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_5_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_6_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__COLOR_7_ASSEMBLY_CNTL(1)
      | VAP_VTX_STATE_CNTL__UPDATE_USER_COLOR_0_ENA(0)
      );

  T0V(GB_ENABLE, 0x00000000);
  T0V(VAP_CNTL_STATUS, 0x00000000);

  T0V(GA_POINT_SIZE
      , GA_POINT_SIZE__HEIGHT(7200)
      | GA_POINT_SIZE__WIDTH(9600)
      );
  T0V(GA_POINT_MINMAX
      , GA_POINT_MINMAX__MIN_SIZE(60)
      | GA_POINT_MINMAX__MAX_SIZE(60)
      );
  T0V(GA_LINE_CNTL
      , GA_LINE_CNTL__WIDTH(6)
      | GA_LINE_CNTL__END_TYPE(2)
      | GA_LINE_CNTL__SORT(0)
      );
  T0V(SU_POLY_OFFSET_ENABLE, 0x00000000);
  T0V(SU_CULL_MODE, 0x00000000);
  T0V(GA_LINE_STIPPLE_CONFIG, 0x00000000);
  T0V(GA_LINE_STIPPLE_VALUE, 0x00000000);
  T0V(GA_POLY_MODE, 0x00000000);
  T0V(GA_ROUND_MODE
      , GA_ROUND_MODE__GEOMETRY_ROUND(1)
      | GA_ROUND_MODE__COLOR_ROUND(0)
      | GA_ROUND_MODE__RGB_CLAMP(1)
      | GA_ROUND_MODE__ALPHA_CLAMP(1)
      | GA_ROUND_MODE__GEOMETRY_MASK(0)
      );
  T0V(SC_CLIP_RULE
      , SC_CLIP_RULE__CLIP_RULE(0xffff));
  T0Vf(GA_POINT_S0, 0.0f);
  T0Vf(GA_POINT_T0, 1.0f);
  T0Vf(GA_POINT_S1, 1.0f);
  T0Vf(GA_POINT_T1, 0.0f);
  T0V(US_OUT_FMT_0
      , US_OUT_FMT__OUT_FMT(0)  // C4_8
      | US_OUT_FMT__C0_SEL(3)   // Blue
      | US_OUT_FMT__C1_SEL(2)   // Green
      | US_OUT_FMT__C2_SEL(1)   // Red
      | US_OUT_FMT__C3_SEL(0)   // Alpha
      | US_OUT_FMT__OUT_SIGN(0)
      );
  T0V(US_OUT_FMT_1
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(US_OUT_FMT_2
      , US_OUT_FMT__OUT_FMT(15) // render target is not used
      );
  T0V(GB_MSPOS0
      , GB_MSPOS0__MS_X0(6)
      | GB_MSPOS0__MS_Y0(6)
      | GB_MSPOS0__MS_X1(6)
      | GB_MSPOS0__MS_Y1(6)
      | GB_MSPOS0__MS_X2(6)
      | GB_MSPOS0__MS_Y2(6)
      | GB_MSPOS0__MSBD0_Y(6)
      | GB_MSPOS0__MSBD0_X(6)
      );
  T0V(GB_MSPOS1
      , GB_MSPOS1__MS_X3(6)
      | GB_MSPOS1__MS_Y3(6)
      | GB_MSPOS1__MS_X4(6)
      | GB_MSPOS1__MS_Y4(6)
      | GB_MSPOS1__MS_X5(6)
      | GB_MSPOS1__MS_Y5(6)
      | GB_MSPOS1__MSBD1(6)
      );
  T0V(US_CONFIG
      , US_CONFIG__ZERO_TIMES_ANYTHING_EQUALS_ZERO(1)
      );
  T0V(US_PIXSIZE
      , US_PIXSIZE__PIX_SIZE(1)
      );
  T0V(US_FC_CTRL, 0);

  T0V(FG_DEPTH_SRC, 0x00000000);
  T0V(US_W_FMT, 0x00000000);
  T0V(VAP_PVS_CONST_CNTL, 0x00000000);
  T0V(TX_INVALTAGS, 0x00000000);
  T0V(TX_ENABLE, 0x00000000);
  T0V(VAP_INDEX_OFFSET, 0x00000000);
  T0V(GA_COLOR_CONTROL
      , GA_COLOR_CONTROL__RGB0_SHADING(2)
      | GA_COLOR_CONTROL__ALPHA0_SHADING(2)
      | GA_COLOR_CONTROL__RGB1_SHADING(2)
      | GA_COLOR_CONTROL__ALPHA1_SHADING(2)
      | GA_COLOR_CONTROL__RGB2_SHADING(2)
      | GA_COLOR_CONTROL__ALPHA2_SHADING(2)
      | GA_COLOR_CONTROL__RGB3_SHADING(2)
      | GA_COLOR_CONTROL__ALPHA3_SHADING(2)
      | GA_COLOR_CONTROL__PROVOKING_VERTEX(3)
      );

  //////////////////////////////////////////////////////////////////////////////
  // CB
  //////////////////////////////////////////////////////////////////////////////

  T0V(RB3D_COLOROFFSET0
      , 0x00000000 // value replaced by kernel from relocs
      );
  T3(_NOP, 0);
  ib[ix++].u32 = 0 * 4; // index into relocs array

  T0V(RB3D_COLORPITCH0
      , RB3D_COLORPITCH__COLORPITCH(1600 >> 1)
      | RB3D_COLORPITCH__COLORFORMAT(6) // ARGB8888
      );
  // The COLORPITCH NOP is ignored/not applied due to
  // RADEON_CS_KEEP_TILING_FLAGS, but is still required.
  T3(_NOP, 0);
  ib[ix++].u32 = 0 * 4; // index into relocs array

  //////////////////////////////////////////////////////////////////////////////
  // SC
  //////////////////////////////////////////////////////////////////////////////

  T0V(SC_SCISSOR0
      , SC_SCISSOR0__XS0(0)
      | SC_SCISSOR0__YS0(0)
      );
  T0V(SC_SCISSOR1
      , SC_SCISSOR1__XS1(1600 - 1)
      | SC_SCISSOR1__YS1(1200 - 1)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0Vf(VAP_VPORT_XSCALE,   800.0f);
  T0Vf(VAP_VPORT_XOFFSET,  800.0f);
  T0Vf(VAP_VPORT_YSCALE,  -600.0f);
  T0Vf(VAP_VPORT_YOFFSET,  600.0f);
  T0Vf(VAP_VPORT_ZSCALE,     0.5f);
  T0Vf(VAP_VPORT_ZOFFSET,    0.5f);

  T0V(VAP_VSM_VTX_ASSM
      , 0x00000001); // undocumented
  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , 0x0);

  T0V(VAP_VF_MAX_VTX_INDX
      , VAP_VF_MAX_VTX_INDX__MAX_INDX(2)
      );

  T0V(VAP_VF_MIN_VTX_INDX
      , VAP_VF_MIN_VTX_INDX__MIN_INDX(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_CODE_CNTL_0
      , VAP_PVS_CODE_CNTL_0__PVS_FIRST_INST(0)
      | VAP_PVS_CODE_CNTL_0__PVS_XYZW_VALID_INST(0)
      | VAP_PVS_CODE_CNTL_0__PVS_LAST_INST(0)
      );
  T0V(VAP_PVS_CODE_CNTL_1
      , VAP_PVS_CODE_CNTL_1__PVS_LAST_VTX_SRC_INST(0)
      );
  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(0)
      );

  const uint32_t vertex_shader[] = {
    0x00f00204,
    0x016da000,
    0x01510001,
    0x01240002,
  };
  const int vertex_shader_length = (sizeof (vertex_shader)) / (sizeof (vertex_shader[0]));
  printf("vs length %d\n", vertex_shader_length);

  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, vertex_shader_length - 1);
  for (int i = 0; i < vertex_shader_length; i++) {
    ib[ix++].u32 = vertex_shader[i];
  }

  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  T0V(RS_IP_0
      , RS_IP__TEX_PTR_S(0)
      | RS_IP__TEX_PTR_T(0)
      | RS_IP__TEX_PTR_R(0)
      | RS_IP__TEX_PTR_Q(0)
      | RS_IP__COL_PTR(0)
      | RS_IP__COL_FMT(6) // Zero components (0,0,0,1)
      | RS_IP__OFFSET_EN(0)
      );
  T0V(RS_COUNT
      , RS_COUNT__IT_COUNT(0)
      | RS_COUNT__IC_COUNT(1)
      | RS_COUNT__W_ADDR(0)
      | RS_COUNT__HIRES_EN(1)
      );
  T0V(RS_INST_COUNT, 0x00000000);
  T0V(RS_INST_0, 0x00000000);

  //////////////////////////////////////////////////////////////////////////////
  // GA_US
  //////////////////////////////////////////////////////////////////////////////

  const uint32_t fragment_shader[] = {
    /*************************************************************
      shader program 0
    *************************************************************/
      US_CMN_INST__TYPE__US_INST_TYPE_OUT
    | US_CMN_INST__TEX_SEM_WAIT(1)
    | US_CMN_INST__RGB_OMASK__RGB
    | US_CMN_INST__ALPHA_OMASK__A

    , US_ALU_RGB_ADDR__ADDR0(128)
    | US_ALU_RGB_ADDR__ADDR1(128)
    | US_ALU_RGB_ADDR__ADDR2(128)

    , US_ALU_ALPHA_ADDR__ADDR0(128)
    | US_ALU_ALPHA_ADDR__ADDR1(128)
    | US_ALU_ALPHA_ADDR__ADDR2(128)

    , US_ALU_RGB_INST__RED_SWIZ_A__ONE
    | US_ALU_RGB_INST__GREEN_SWIZ_A__ONE
    | US_ALU_RGB_INST__BLUE_SWIZ_A__ZERO
    | US_ALU_RGB_INST__RED_SWIZ_B__ONE
    | US_ALU_RGB_INST__GREEN_SWIZ_B__ONE
    | US_ALU_RGB_INST__BLUE_SWIZ_B__ZERO
    | US_ALU_RGB_INST__OMOD(7)
    | US_ALU_RGB_INST__TARGET__A

    , US_ALU_ALPHA_INST__ALPHA_OP__OP_MAX
    | US_ALU_ALPHA_INST__ALPHA_SWIZ_A__ONE
    | US_ALU_ALPHA_INST__ALPHA_SWIZ_B__ONE
    | US_ALU_ALPHA_INST__OMOD(7)
    | US_ALU_ALPHA_INST__TARGET__A

    , US_ALU_RGBA_INST__RGB_OP__OP_MAX

    /*************************************************************
      shader program 1
    *************************************************************/
    , US_CMN_INST__TYPE__US_INST_TYPE_OUT
    | US_CMN_INST__TEX_SEM_WAIT(1)
    | US_CMN_INST__RGB_OMASK__RGB
    | US_CMN_INST__ALPHA_OMASK__A

    , US_ALU_RGB_ADDR__ADDR0(128)
    | US_ALU_RGB_ADDR__ADDR1(128)
    | US_ALU_RGB_ADDR__ADDR2(128)

    , US_ALU_ALPHA_ADDR__ADDR0(128)
    | US_ALU_ALPHA_ADDR__ADDR1(128)
    | US_ALU_ALPHA_ADDR__ADDR2(128)

    , US_ALU_RGB_INST__RED_SWIZ_A__ONE
    | US_ALU_RGB_INST__GREEN_SWIZ_A__ONE
    | US_ALU_RGB_INST__BLUE_SWIZ_A__ONE
    | US_ALU_RGB_INST__RED_SWIZ_B__ONE
    | US_ALU_RGB_INST__GREEN_SWIZ_B__ONE
    | US_ALU_RGB_INST__BLUE_SWIZ_B__ONE
    | US_ALU_RGB_INST__OMOD(7)
    | US_ALU_RGB_INST__TARGET__A

    , US_ALU_ALPHA_INST__ALPHA_OP__OP_MAX
    | US_ALU_ALPHA_INST__ALPHA_SWIZ_A__ONE
    | US_ALU_ALPHA_INST__ALPHA_SWIZ_B__ONE
    | US_ALU_ALPHA_INST__OMOD(7)
    | US_ALU_ALPHA_INST__TARGET__A

    , US_ALU_RGBA_INST__RGB_OP__OP_MAX
  };
  const int fragment_shader_length = (sizeof (fragment_shader)) / (sizeof (fragment_shader[0]));
  printf("fs length %d\n", fragment_shader_length);

  T0V(GA_US_VECTOR_INDEX, 0x00000000);
  T0_ONE_REG(GA_US_VECTOR_DATA, fragment_shader_length - 1);
  for (int i = 0; i < fragment_shader_length; i++) {
    ib[ix++].u32 = fragment_shader[i];
  }

  return ix;
}

int indirect_buffer_render(int ix, int colorbuffer_ix) {
  //////////////////////////////////////////////////////////////////////////////
  // CB
  //////////////////////////////////////////////////////////////////////////////

  T0V(RB3D_COLOROFFSET0
      , 0x00000000 // value replaced by kernel from relocs
      );
  T3(_NOP, 0);
  ib[ix++].u32 = colorbuffer_ix * 4; // index into relocs array

  T0V(RB3D_COLORPITCH0
      , RB3D_COLORPITCH__COLORPITCH(1600 >> 1)
      | RB3D_COLORPITCH__COLORFORMAT(6) // ARGB8888
      );
  // The COLORPITCH NOP is ignored/not applied due to
  // RADEON_CS_KEEP_TILING_FLAGS, but is still required.
  T3(_NOP, 0);
  ib[ix++].u32 = colorbuffer_ix * 4; // index into relocs array

  //////////////////////////////////////////////////////////////////////////////
  // CLEAR
  //////////////////////////////////////////////////////////////////////////////

  T0V(US_CODE_OFFSET
      , US_CODE_OFFSET__OFFSET_ADDR(1)
      );

  T0V(US_CODE_ADDR
      , US_CODE_ADDR__START_ADDR(0)
      | US_CODE_ADDR__END_ADDR(0)
      );

  T0V(US_CODE_RANGE
      , US_CODE_RANGE__CODE_ADDR(1)
      | US_CODE_RANGE__CODE_SIZE(0)
      );

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__CLIP_DISABLE(1)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VTX_XY_FMT(1)
      | VAP_VTE_CNTL__VTX_Z_FMT(1)
      );

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0(1) // float 2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0(0)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0(1)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0(5)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0(5)
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(15)
      );

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(2)
      );

  const float center[] = {
    800.0f, 600.0f,
  };
  T3(_3D_DRAW_IMMD_2, (1 + 2) - 1);
  ib[ix++].u32
    = VAP_VF_CNTL__PRIM_TYPE(1) // point list
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(1)
    ;
  for (int i = 0; i < 2; i++) {
    ib[ix++].f32 = center[i];
  }

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  T0V(US_CODE_OFFSET
      , US_CODE_OFFSET__OFFSET_ADDR(0)
      );

  T0V(US_CODE_ADDR
      , US_CODE_ADDR__START_ADDR(0)
      | US_CODE_ADDR__END_ADDR(0)
      );

  T0V(US_CODE_RANGE
      , US_CODE_RANGE__CODE_ADDR(0)
      | US_CODE_RANGE__CODE_SIZE(0)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VPORT_X_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_X_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Y_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_SCALE_ENA(1)
      | VAP_VTE_CNTL__VPORT_Z_OFFSET_ENA(1)
      | VAP_VTE_CNTL__VTX_XY_FMT(0)
      | VAP_VTE_CNTL__VTX_Z_FMT(0)
      | VAP_VTE_CNTL__VTX_W0_FMT(1)
      | VAP_VTE_CNTL__SERIAL_PROC_ENA(0)
      );

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1024)
      );

  float translate[4] = {
    sinf(translate_x), 0, 0, 0,
  };
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, (4 - 1));
  for (int i = 0; i < 4; i++)
    ib[ix++].f32 = translate[i];

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__PS_UCP_MODE(3)
      );

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0(2) // float 3
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0(0)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0(1)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0(2)
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0(5)
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(15)
      );

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(3)
      );

  const float vertices[] = {
    // position
     0.5f, -0.5f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, // bottom left
     0.0f,  0.5f, 0.0f, // top
  };
  const int vertices_length = (sizeof (vertices)) / (sizeof (vertices[0]));
  printf("vtx length %d\n", vertices_length);
  T3(_3D_DRAW_IMMD_2, (1 + vertices_length) - 1);
  ib[ix++].u32
    = VAP_VF_CNTL__PRIM_TYPE(4)
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(3)
    ;
  for (int i = 0; i < vertices_length; i++) {
    ib[ix++].f32 = vertices[i];
  }

  return ix;
}

int indirect_buffer_padding(int ix)
{
  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ix % 8) != 0) {
    ib[ix++].u32 = 0x80000000;
  }

  return ix;
}

int main()
{
  //////////////////////////////////////////////////////////////////////////////
  // PCI resource0
  //////////////////////////////////////////////////////////////////////////////
  const char * resource2_path = "/sys/bus/pci/devices/0000:01:00.0/resource2";
  int resource2_fd = open(resource2_path, O_RDWR | O_SYNC);
  assert(resource2_fd >= 0);

  uint32_t resource2_size = 0x10000;
  void * resource2_base = mmap(0, resource2_size, PROT_READ | PROT_WRITE, MAP_SHARED, resource2_fd, 0);
  assert(resource2_base != MAP_FAILED);

  void * rmmio = resource2_base;

  //////////////////////////////////////////////////////////////////////////////
  // DRI card0
  //////////////////////////////////////////////////////////////////////////////

  int ret;
  int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

  const int colorbuffer_size = 1600 * 1200 * 4;
  int colorbuffer_handle[2];
  void * colorbuffer_ptr[2];
  int flush_handle;

  // colorbuffers
  {
    struct drm_radeon_gem_create args = {
      .size = colorbuffer_size,
      .alignment = 4096,
      .handle = 0,
      .initial_domain = 4, // RADEON_GEM_DOMAIN_VRAM
      .flags = 4
    };

    for (int i = 0; i < 2; i++) {
      args.handle = 0;

      ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE, &args, (sizeof (struct drm_radeon_gem_create)));
      if (ret != 0) {
        perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
      }
      assert(args.handle != 0);

      colorbuffer_handle[i] = args.handle;
    }
  }

  {
    for (int i = 0; i < 2; i++) {
      struct drm_radeon_gem_mmap mmap_args = {
        .handle = colorbuffer_handle[i],
        .offset = 0,
        .size = colorbuffer_size,
      };
      ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_MMAP, &mmap_args, (sizeof (struct drm_radeon_gem_mmap)));
      if (ret != 0) {
        perror("drmCommandWriteRead(DRM_RADEON_GEM_MMAP)");
      }

      colorbuffer_ptr[i] = mmap(0, mmap_args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
                                fd, mmap_args.addr_ptr);
      assert(colorbuffer_ptr[i] != MAP_FAILED);
    }
  }

  { // clear colorbuffers
    for (int i = 0; i < 2; i++) {
      for (int off = 0; off < colorbuffer_size / 4; off++) {
        ((uint32_t*)(colorbuffer_ptr[i]))[off] = 0;
      }
    }
    asm volatile ("" ::: "memory");
  }

  // flush
  {
    struct drm_radeon_gem_create args = {
      .size = 4096,
      .alignment = 4096,
      .handle = 0,
      .initial_domain = 2, // GTT
      .flags = 0
    };

    ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE,
                              &args, (sizeof (args)));
    if (ret != 0) {
      perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
    }
    assert(args.handle != 0);
    flush_handle = args.handle;
  }

  fprintf(stderr, "colorbuffer_handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer_handle[1] %d\n", colorbuffer_handle[1]);

  struct drm_radeon_cs_reloc relocs[] = {
    {
      .handle = colorbuffer_handle[0],
      .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
      .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
      .flags = 8,
    },
    {
      .handle = colorbuffer_handle[1],
      .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
      .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
      .flags = 8,
    },
    {
      .handle = flush_handle,
      .read_domains = 2, // RADEON_GEM_DOMAIN_GTT
      .write_domain = 2, // RADEON_GEM_DOMAIN_GTT
      .flags = 0,
    }
  };

  uint32_t flags[2] = {
    5, // RADEON_CS_KEEP_TILING_FLAGS | RADEON_CS_END_OF_FRAME
    0, // RADEON_CS_RING_GFX
  };

  int ib_dwords = 0;
  int colorbuffer_ix = 0;

  ib_dwords = indirect_buffer_setup(ib_dwords);
  ib_dwords = indirect_buffer_render(ib_dwords, colorbuffer_ix);
  ib_dwords = indirect_buffer_padding(ib_dwords);

  while (true) {
    struct drm_radeon_cs_chunk chunks[3] = {
      {
        .chunk_id = RADEON_CHUNK_ID_IB,
        .length_dw = ib_dwords,
        .chunk_data = (uint64_t)(uintptr_t)ib,
      },
      {
        .chunk_id = RADEON_CHUNK_ID_RELOCS,
        .length_dw = (sizeof (relocs)) / (sizeof (uint32_t)),
        .chunk_data = (uint64_t)(uintptr_t)relocs,
      },
      {
        .chunk_id = RADEON_CHUNK_ID_FLAGS,
        .length_dw = (sizeof (flags)) / (sizeof (uint32_t)),
        .chunk_data = (uint64_t)(uintptr_t)&flags,
      },
    };

    uint64_t chunks_array[3] = {
      (uint64_t)(uintptr_t)&chunks[0],
      (uint64_t)(uintptr_t)&chunks[1],
      (uint64_t)(uintptr_t)&chunks[2],
    };

    struct drm_radeon_cs cs = {
      .num_chunks = 3,
      .cs_id = 0,
      .chunks = (uint64_t)(uintptr_t)chunks_array,
      .gart_limit = 0,
      .vram_limit = 0,
    };

    ret = drmCommandWriteRead(fd, DRM_RADEON_CS, &cs, (sizeof (struct drm_radeon_cs)));
    if (ret != 0) {
      perror("drmCommandWriteRead(DRM_RADEON_CS)");
      break;
    }

    /*
    struct drm_radeon_gem_wait_idle args = {
      .handle = flush_handle
    };
    while (drmCommandWrite(fd, DRM_RADEON_GEM_WAIT_IDLE, &args, (sizeof (struct drm_radeon_gem_wait_idle))) == -EBUSY);
    */

#define D1CRTC_DOUBLE_BUFFER_CONTROL 0x60ec
#define D1GRPH_PRIMARY_SURFACE_ADDRESS 0x6110
#define D1GRPH_UPDATE 0x6144
#define D1GRPH_UPDATE__D1GRPH_SURFACE_UPDATE_PENDING (1 << 2)

    printf("D1CRTC_DOUBLE_BUFFER_CONTROL: %08x\n", rreg(rmmio, D1CRTC_DOUBLE_BUFFER_CONTROL));

    // addresses were retrieved from /sys/kernel/debug/radeon_vram_mm
    //
    // This assumes GEM buffer allocation always starts from the lowest
    // unallocated address.
    const uint32_t colorbuffer_addresses[2] = {
      0x813000,
      0xf66000,
    };

    wreg(rmmio, D1GRPH_PRIMARY_SURFACE_ADDRESS, colorbuffer_addresses[!colorbuffer_ix]);
    while ((rreg(rmmio, D1GRPH_UPDATE) & D1GRPH_UPDATE__D1GRPH_SURFACE_UPDATE_PENDING) != 0);

    // next state
    translate_x += 0.1f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;

    // next indirect buffer
    ib_dwords = 0;
    ib_dwords = indirect_buffer_setup(ib_dwords);
    ib_dwords = indirect_buffer_render(ib_dwords, colorbuffer_ix);
    ib_dwords = indirect_buffer_padding(ib_dwords);
  }

  {
    int out_fd = open("colorbuffer0.data", O_RDWR|O_CREAT, 0644);
    assert(out_fd >= 0);
    ssize_t write_length = write(out_fd, colorbuffer_ptr[0], colorbuffer_size);
    assert(write_length == colorbuffer_size);
    close(out_fd);
  }

  {
    int out_fd = open("colorbuffer1.data", O_RDWR|O_CREAT, 0644);
    assert(out_fd >= 0);
    ssize_t write_length = write(out_fd, colorbuffer_ptr[1], colorbuffer_size);
    assert(write_length == colorbuffer_size);
    close(out_fd);
  }

  int mm_fd = open("/sys/kernel/debug/radeon_vram_mm", O_RDONLY);
  assert(mm_fd >= 0);
  char buf[4096];
  while (true) {
    ssize_t read_length = read(mm_fd, buf, 4096);
    assert(read_length >= 0);
    ssize_t ret = write(STDOUT_FILENO, buf, read_length);
    (void)ret;
    if (read_length < 4096) {
      break;
    }
  }
  close(mm_fd);

  munmap(colorbuffer_ptr, colorbuffer_size);

  close(fd);
}
