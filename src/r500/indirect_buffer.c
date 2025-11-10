#include <assert.h>
#include <stdio.h>

#include "command_processor.h"
#include "shader.h"
#include "indirect_buffer.h"

#include "3d_registers.h"
#include "3d_registers_undocumented.h"
#include "3d_registers_bits.h"

union u32_f32 ib[16384 * 100];
volatile int ib_ix;

void ib_generic_initialization()
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

  // anti-aliasing

  T0V(GB_AA_CONFIG, 0x00000000);
  T0V(RB3D_AARESOLVE_CTL, 0x00000000);

  // z buffer

  T0V(ZB_BW_CNTL, 0x00000000);
  T0V(ZB_DEPTHCLEARVALUE, 0x00000000);
  T0V(ZB_ZTOP
      , ZB_ZTOP__ZTOP(1)
      );
  T0V(ZB_STENCILREFMASK, 0x00000000);
  T0V(ZB_STENCILREFMASK_BF, 0x00000000);

  // fog

  T0V(FG_ALPHA_FUNC, 0x00000000);
  T0V(FG_ALPHA_VALUE, 0x00000000);
  T0V(FG_FOG_BLEND, 0x00000000);
  T0V(FG_DEPTH_SRC, 0x00000000);

  // render backend

  T0V(RB3D_CCTL
      , RB3D_CCTL__INDEPENDENT_COLORFORMAT_ENABLE(1)
      );
  T0V(RB3D_ROPCNTL, 0x00000000);
  T0V(RB3D_COLOR_CHANNEL_MASK
      , RB3D_COLOR_CHANNEL_MASK__BLUE_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__GREEN_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__RED_MASK(1)
      | RB3D_COLOR_CHANNEL_MASK__ALPHA_MASK(1)
      );
  T0V(RB3D_DITHER_CTL, 0x00000000);
  T0V(RB3D_CONSTANT_COLOR_AR
      , RB3D_CONSTANT_COLOR_AR__RED(0)
      | RB3D_CONSTANT_COLOR_AR__ALPHA(0)
      );
  T0V(RB3D_CONSTANT_COLOR_GB
      , RB3D_CONSTANT_COLOR_GB__BLUE(0)
      | RB3D_CONSTANT_COLOR_GB__GREEN(0)
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

  // clip

  T0V(SC_HYPERZ_EN, 0x00000000);

  T0V(SC_CLIP_RULE
      , SC_CLIP_RULE__CLIP_RULE(0xffff));

  T0V(SC_CLIP_0_A, 0x00000000);
  T0V(SC_CLIP_0_B, 0xffffffff);
  T0V(SC_SCREENDOOR, 0x00ffffff);
  T0V(SC_EDGERULE
      , SC_EDGERULE__ER_TRI(5)      // L-in,R-out,HT-in,HB-in
      | SC_EDGERULE__ER_POINT(9)    // L-out,R-in,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_LR(5)  // L-in,R-out,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_RL(9)  // L-out,R-in,HT-in,HB-out
      | SC_EDGERULE__ER_LINE_TB(26) // T-in,B-out,VL-out,VR-in
      | SC_EDGERULE__ER_LINE_BT(22) // T-out,B-in,VL-out,VR-in
      );

  // ga

  T0V(GA_OFFSET
      , GA_OFFSET__X_OFFSET(0)
      | GA_OFFSET__Y_OFFSET(0)
      );

  T0V(GA_COLOR_CONTROL_PS3, 0x00000000);

  T0V(GA_POINT_MINMAX
      , GA_POINT_MINMAX__MIN_SIZE(60)
      | GA_POINT_MINMAX__MAX_SIZE(60)
      );

  T0V(GA_LINE_CNTL
      , GA_LINE_CNTL__WIDTH(6)
      | GA_LINE_CNTL__END_TYPE(2)
      | GA_LINE_CNTL__SORT(0)
      );

  T0V(GA_LINE_STIPPLE_CONFIG, 0x00000000);
  T0V(GA_LINE_STIPPLE_VALUE, 0x00000000);
  T0V(GA_POLY_MODE
      , GA_POLY_MODE__POLY_MODE(0));
  T0V(GA_ROUND_MODE
      , GA_ROUND_MODE__GEOMETRY_ROUND(1)
      | GA_ROUND_MODE__COLOR_ROUND(0)
      | GA_ROUND_MODE__RGB_CLAMP(1)
      | GA_ROUND_MODE__ALPHA_CLAMP(1)
      | GA_ROUND_MODE__GEOMETRY_MASK(0)
      );

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

  // gb

  T0V(GB_Z_PEQ_CONFIG
      , GB_Z_PEQ_CONFIG__Z_PEQ_SIZE(0) // 4x4 z plane equations
      );

  T0V(GB_SELECT, 0x00000000);

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

  // setup unit

  T0V(SU_TEX_WRAP, 0x00000000);
  T0V(SU_TEX_WRAP_PS3, 0x00000000);

  T0Vf(SU_DEPTH_SCALE, 16777215.0f);
  T0Vf(SU_DEPTH_OFFSET, 0.0f);

  T0V(SU_CULL_MODE
      , SU_CULL_MODE__CULL_FRONT(0)
      | SU_CULL_MODE__CULL_BACK(0)
      | SU_CULL_MODE__FACE(0)
      );

  T0V(SU_POLY_OFFSET_ENABLE, 0x00000000);

  // VAP

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

  // vap constants

  T0(VAP_PVS_FLOW_CNTL_ADDRS_LW_0, 31);
  for (int i = 0; i < 32; i++)
    TU(0x00000000);

  T0(VAP_PVS_FLOW_CNTL_LOOP_INDEX_0, 15);
  for (int i = 0; i < 16; i++)
    TU(0x00000000);

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1536));
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, 23);
  for (int i = 0; i < 24; i++)
    TU(0x00000000);

  // us

  T0V(US_CONFIG
      , US_CONFIG__ZERO_TIMES_ANYTHING_EQUALS_ZERO(1)
      );
  T0V(US_FC_CTRL, 0);

  T0V(US_W_FMT
      , US_W_FMT__W_FMT(0) // W is always zero
      );
}

void ib_colorbuffer(int reloc_index, int pitch,
                    int macrotile, int microtile)
{

  //////////////////////////////////////////////////////////////////////////////
  // CB
  //////////////////////////////////////////////////////////////////////////////

  T0V(RB3D_COLOROFFSET0
      , 0x00000000 // value replaced by kernel from relocs
      );
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array

  T0V(RB3D_COLORPITCH0
      , RB3D_COLORPITCH__COLORPITCH(pitch >> 1)
      | RB3D_COLORPITCH__COLORTILE(macrotile)
      | RB3D_COLORPITCH__COLORMICROTILE(microtile)
      | RB3D_COLORPITCH__COLORFORMAT__ARGB8888
      );
  // The COLORPITCH NOP is ignored/not applied due to
  // RADEON_CS_KEEP_TILING_FLAGS, but is still required.
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_colorbuffer2(int buffer_index,
                     int reloc_index,
                     int pitch,
                     int macrotile, int microtile,
                     int colorformat)
{
  assert(buffer_index >= 0 && buffer_index <= 3);

  int reg_offset = buffer_index * 4;

  //////////////////////////////////////////////////////////////////////////////
  // CB
  //////////////////////////////////////////////////////////////////////////////

  T0V(RB3D_COLOROFFSET0 + reg_offset
      , 0x00000000 // value replaced by kernel from relocs
      );
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array

  T0V(RB3D_COLORPITCH0 + reg_offset
      , RB3D_COLORPITCH__COLORPITCH(pitch >> 1)
      | RB3D_COLORPITCH__COLORTILE(macrotile)
      | RB3D_COLORPITCH__COLORMICROTILE(microtile)
      | RB3D_COLORPITCH__COLORFORMAT(colorformat)
      );
  // The COLORPITCH NOP is ignored/not applied due to
  // RADEON_CS_KEEP_TILING_FLAGS, but is still required.
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_colorbuffer3(int buffer_index,
                     int reloc_index,
                     int offset,
                     int pitch,
                     int macrotile, int microtile,
                     int colorformat)
{
  assert(buffer_index >= 0 && buffer_index <= 3);

  int reg_offset = buffer_index * 4;

  //////////////////////////////////////////////////////////////////////////////
  // CB
  //////////////////////////////////////////////////////////////////////////////

  T0V(RB3D_COLOROFFSET0 + reg_offset
      , offset // value replaced by kernel from relocs
      );
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array

  T0V(RB3D_COLORPITCH0 + reg_offset
      , RB3D_COLORPITCH__COLORPITCH(pitch >> 1)
      | RB3D_COLORPITCH__COLORTILE(macrotile)
      | RB3D_COLORPITCH__COLORMICROTILE(microtile)
      | RB3D_COLORPITCH__COLORFORMAT(colorformat)
      );
  // The COLORPITCH NOP is ignored/not applied due to
  // RADEON_CS_KEEP_TILING_FLAGS, but is still required.
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_viewport(int width, int height)
{
  //////////////////////////////////////////////////////////////////////////////
  // SC
  //////////////////////////////////////////////////////////////////////////////

  T0V(SC_SCISSOR0
      , SC_SCISSOR0__XS0(0)
      | SC_SCISSOR0__YS0(0)
      );
  T0V(SC_SCISSOR1
      , SC_SCISSOR1__XS1(width - 1)
      | SC_SCISSOR1__YS1(height - 1)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  float x = ((float)width) * 0.5f;
  float y = ((float)height) * 0.5f;

  T0Vf(VAP_VPORT_XSCALE,   x);
  T0Vf(VAP_VPORT_XOFFSET,  x);
  T0Vf(VAP_VPORT_YSCALE,  -y);
  T0Vf(VAP_VPORT_YOFFSET,  y);
  T0Vf(VAP_VPORT_ZSCALE,   0.5f);
  T0Vf(VAP_VPORT_ZOFFSET,  0.5f);
}

void ib_zbuffer(int reloc_index, int pitch, int zfunc)
{
  //////////////////////////////////////////////////////////////////////////////
  // ZB
  //////////////////////////////////////////////////////////////////////////////
  T0V(ZB_CNTL
      , ZB_CNTL__Z_ENABLE__ENABLED // 1
      | ZB_CNTL__ZWRITEENABLE__ENABLE // 1
      );
  T0V(ZB_ZSTENCILCNTL
      , ZB_ZSTENCILCNTL__ZFUNC(zfunc)
      );

  T0V(ZB_FORMAT
      , ZB_FORMAT__DEPTHFORMAT(2) // 24-bit integer Z, 8 bit stencil
      );

  T0V(ZB_DEPTHOFFSET, 0);
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array

  T0V(ZB_DEPTHPITCH
      , ZB_DEPTHPITCH__DEPTHPITCH(pitch >> 2)
      | ZB_DEPTHPITCH__DEPTHMACROTILE(1)
      | ZB_DEPTHPITCH__DEPTHMICROTILE(1)
      );
  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_rs_instructions(int count)
{
  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  assert(count <= 8 && count >= 0);

  if (count == 0) {
    T0V(RS_COUNT
        , RS_COUNT__IT_COUNT(0)
        | RS_COUNT__IC_COUNT(1)
        | RS_COUNT__W_ADDR(0)
        | RS_COUNT__HIRES_EN(1)
        );
    T0V(RS_INST_COUNT
        , RS_INST_COUNT__INST_COUNT(0));
  } else {
    T0V(RS_COUNT
        , RS_COUNT__IT_COUNT(count * 4)
        | RS_COUNT__IC_COUNT(0)
        | RS_COUNT__W_ADDR(0)
        | RS_COUNT__HIRES_EN(1)
        );
    T0V(RS_INST_COUNT
        , RS_INST_COUNT__INST_COUNT(count - 1));
  }

  switch (count) {
  case 8:
    T0V(RS_IP_7
        , RS_IP__TEX_PTR_S(28)
        | RS_IP__TEX_PTR_T(29)
        | RS_IP__TEX_PTR_R(30)
        | RS_IP__TEX_PTR_Q(31)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_7
        , RS_INST__TEX_ID(7)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(7)
        );
    [[fallthrough]];
  case 7:
    T0V(RS_IP_6
        , RS_IP__TEX_PTR_S(24)
        | RS_IP__TEX_PTR_T(25)
        | RS_IP__TEX_PTR_R(26)
        | RS_IP__TEX_PTR_Q(27)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_6
        , RS_INST__TEX_ID(6)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(6)
        );
    [[fallthrough]];
  case 6:
    T0V(RS_IP_5
        , RS_IP__TEX_PTR_S(20)
        | RS_IP__TEX_PTR_T(21)
        | RS_IP__TEX_PTR_R(22)
        | RS_IP__TEX_PTR_Q(23)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_5
        , RS_INST__TEX_ID(5)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(5)
        );
    [[fallthrough]];
  case 5:
    T0V(RS_IP_4
        , RS_IP__TEX_PTR_S(16)
        | RS_IP__TEX_PTR_T(17)
        | RS_IP__TEX_PTR_R(18)
        | RS_IP__TEX_PTR_Q(19)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_4
        , RS_INST__TEX_ID(4)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(4)
        );
    [[fallthrough]];
  case 4:
    T0V(RS_IP_3
        , RS_IP__TEX_PTR_S(12)
        | RS_IP__TEX_PTR_T(13)
        | RS_IP__TEX_PTR_R(14)
        | RS_IP__TEX_PTR_Q(15)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_3
        , RS_INST__TEX_ID(3)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(3)
        );
    [[fallthrough]];
  case 3:
    T0V(RS_IP_2
        , RS_IP__TEX_PTR_S(8)
        | RS_IP__TEX_PTR_T(9)
        | RS_IP__TEX_PTR_R(10)
        | RS_IP__TEX_PTR_Q(11)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_2
        , RS_INST__TEX_ID(2)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(2)
        );
    [[fallthrough]];
  case 2:
    T0V(RS_IP_1
        , RS_IP__TEX_PTR_S(4)
        | RS_IP__TEX_PTR_T(5)
        | RS_IP__TEX_PTR_R(6)
        | RS_IP__TEX_PTR_Q(7)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_1
        , RS_INST__TEX_ID(1)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(1)
        );
    [[fallthrough]];
  case 1:
    T0V(RS_IP_0
        , RS_IP__TEX_PTR_S(0)
        | RS_IP__TEX_PTR_T(1)
        | RS_IP__TEX_PTR_R(2)
        | RS_IP__TEX_PTR_Q(3)
        | RS_IP__OFFSET_EN(0)
        );
    T0V(RS_INST_0
        , RS_INST__TEX_ID(0)
        | RS_INST__TEX_CN(1)
        | RS_INST__TEX_ADDR(0)
        );
    break;
  case 0:
    T0V(RS_IP_0
        , RS_IP__COL_PTR(0)
        | RS_IP__COL_FMT(6) // Zero components (0,0,0,1)
        );
    break;
  }
}

void ib_texture__0()
{
  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE, 0x00000000);
}

void ib_texture__1(int reloc_index,
                   int width, int height,
                   int macrotile, int microtile,
                   int clamp)
{
  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE
      , TX_ENABLE__TEX_0_ENABLE__ENABLE);

  T0V(TX_FILTER0_0
      , TX_FILTER0__CLAMP_S(clamp)
      | TX_FILTER0__CLAMP_T(clamp)
      | TX_FILTER0__MAG_FILTER__LINEAR
      | TX_FILTER0__MIN_FILTER__LINEAR
      );
  T0V(TX_FILTER1_0
      , TX_FILTER1__LOD_BIAS(1)
      | TX_FILTER1__BORDER_FIX(1)
      );
  T0V(TX_BORDER_COLOR_0, 0);
  T0V(TX_FORMAT0_0
      , TX_FORMAT0__TXWIDTH(width - 1)
      | TX_FORMAT0__TXHEIGHT(height - 1)
      );

  T0V(TX_FORMAT1_0
      , TX_FORMAT1__TXFORMAT__TX_FMT_8_8_8_8
      | TX_FORMAT1__SEL_ALPHA(5)
      | TX_FORMAT1__SEL_RED(0)
      | TX_FORMAT1__SEL_GREEN(1)
      | TX_FORMAT1__SEL_BLUE(2)
      | TX_FORMAT1__TEX_COORD_TYPE__2D
      );
  T0V(TX_FORMAT2_0, 0);

  T0V(TX_OFFSET_0
      , TX_OFFSET__MACRO_TILE(macrotile)
      | TX_OFFSET__MICRO_TILE(microtile)
      );

  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_texture__1_float32(int reloc_index,
                           int width, int height,
                           int macrotile, int microtile,
                           int clamp)
{
  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE
      , TX_ENABLE__TEX_0_ENABLE__ENABLE);

  T0V(TX_FILTER0_0
      , TX_FILTER0__CLAMP_S(clamp)
      | TX_FILTER0__CLAMP_T(clamp)
      | TX_FILTER0__MAG_FILTER__POINT
      | TX_FILTER0__MIN_FILTER__POINT
      | TX_FILTER0__ID(0)
      );
  T0V(TX_FILTER1_0
      , TX_FILTER1__LOD_BIAS(1)
      | TX_FILTER1__BORDER_FIX(1)
      );
  T0V(TX_BORDER_COLOR_0, 0);
  T0V(TX_FORMAT0_0
      , TX_FORMAT0__TXWIDTH(width - 1)
      | TX_FORMAT0__TXHEIGHT(height - 1)
      );

  T0V(TX_FORMAT1_0
      , TX_FORMAT1__TXFORMAT__TX_FMT_32F_32F_32F_32F
      | TX_FORMAT1__SEL_ALPHA(3)
      | TX_FORMAT1__SEL_RED(0)
      | TX_FORMAT1__SEL_GREEN(1)
      | TX_FORMAT1__SEL_BLUE(2)
      | TX_FORMAT1__TEX_COORD_TYPE__2D
      );
  T0V(TX_FORMAT2_0, 0);

  T0V(TX_OFFSET_0
      , TX_OFFSET__MACRO_TILE(macrotile)
      | TX_OFFSET__MICRO_TILE(microtile)
      );

  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_texture2(int texture_index,
                 int reloc_index,
                 int width, int height,
                 int macrotile, int microtile,
                 int clamp,
                 int txformat)
{
  assert(texture_index >= 0 && texture_index <= 15);

  int texture_offset = texture_index * 4;

  T0V(TX_FILTER0_0 + texture_offset
      , TX_FILTER0__CLAMP_S(clamp)
      | TX_FILTER0__CLAMP_T(clamp)
      | TX_FILTER0__MAG_FILTER__POINT
      | TX_FILTER0__MIN_FILTER__POINT
      | TX_FILTER0__ID(texture_index)
      );
  T0V(TX_FILTER1_0 + texture_offset
      , TX_FILTER1__LOD_BIAS(1)
      | TX_FILTER1__BORDER_FIX(0)
      );
  T0V(TX_BORDER_COLOR_0 + texture_offset
      , 0
      );
  T0V(TX_FORMAT0_0 + texture_offset
      , TX_FORMAT0__TXWIDTH(width - 1)
      | TX_FORMAT0__TXHEIGHT(height - 1)
      );

  T0V(TX_FORMAT1_0 + texture_offset
      , TX_FORMAT1__TXFORMAT(txformat)
      | TX_FORMAT1__SEL_ALPHA(3)
      | TX_FORMAT1__SEL_RED(0)
      | TX_FORMAT1__SEL_GREEN(1)
      | TX_FORMAT1__SEL_BLUE(2)
      | TX_FORMAT1__TEX_COORD_TYPE__2D
      );
  T0V(TX_FORMAT2_0 + texture_offset
      , 0
      );

  T0V(TX_OFFSET_0 + texture_offset
      , TX_OFFSET__MACRO_TILE(macrotile)
      | TX_OFFSET__MICRO_TILE(microtile)
      );

  T3(_NOP, 0);
  TU(reloc_index * 4); // index into relocs array
}

void ib_vap_pvs(struct shader_offset * offset)
{
  const int instruction_size = 4 * 4; // bytes
  int first_inst = offset->start / instruction_size;
  int last_inst = ((offset->start + offset->size) / instruction_size) - 1;
  assert(last_inst >= first_inst);

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_CODE_CNTL_0
      , VAP_PVS_CODE_CNTL_0__PVS_FIRST_INST(first_inst)
      | VAP_PVS_CODE_CNTL_0__PVS_XYZW_VALID_INST(last_inst)
      | VAP_PVS_CODE_CNTL_0__PVS_LAST_INST(last_inst)
      );
  T0V(VAP_PVS_CODE_CNTL_1
      , VAP_PVS_CODE_CNTL_1__PVS_LAST_VTX_SRC_INST(last_inst)
      );
}

void ib_ga_us(struct shader_offset * offset)
{
  const int instruction_size = 4 * 6; // bytes
  int code_addr = offset->start / instruction_size;
  int code_size = offset->size / instruction_size;

  //////////////////////////////////////////////////////////////////////////////
  // GA_US
  //////////////////////////////////////////////////////////////////////////////

  T0V(US_CODE_RANGE
      , US_CODE_RANGE__CODE_ADDR(code_addr)
      | US_CODE_RANGE__CODE_SIZE(code_size - 1)  // relative to CODE_ADDR
      );
  T0V(US_CODE_OFFSET
      , US_CODE_OFFSET__OFFSET_ADDR(code_addr)
      );
  T0V(US_CODE_ADDR
      , US_CODE_ADDR__START_ADDR(0)             // relative to OFFSET_ADDR
      | US_CODE_ADDR__END_ADDR(code_size - 1)   // relative to OFFSET_ADDR
      );
}

void ib_vap_pvs_const_cntl(const float * consts, int size)
{
  assert(size % 16 == 0);

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS_CONST_CNTL
  //////////////////////////////////////////////////////////////////////////////

  const int consts_length = size / 4;

  T0V(VAP_PVS_CONST_CNTL
      , VAP_PVS_CONST_CNTL__PVS_CONST_BASE_OFFSET(0)
      | VAP_PVS_CONST_CNTL__PVS_MAX_CONST_ADDR(consts_length - 1)
      );

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1024)
      );

  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, (consts_length - 1));
  for (int i = 0; i < consts_length; i++)
    TF(consts[i]);
}

void ib_vap_pvs_const_offset(const float * consts, int size, int offset)
{
  assert(size % 16 == 0);

  const int consts_length = size / 4;

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1024 + offset)
      );

  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, (consts_length - 1));
  for (int i = 0; i < consts_length; i++)
    TF(consts[i]);
}

void ib_ga_consts(const float * consts, int consts_length, int index)
{
  assert(consts_length % 4 == 0);

  T0V(GA_US_VECTOR_INDEX
      , GA_US_VECTOR_INDEX__INDEX(index)
      | GA_US_VECTOR_INDEX__TYPE(1)
      );
  T0_ONE_REG(GA_US_VECTOR_DATA, (consts_length - 1));
  for (int i = 0; i < consts_length; i++)
    TF(consts[i]);
}

void ib_vap_stream_cntl__2()
{
  //////////////////////////////////////////////////////////////////////////////
  // VAP_PROG_STREAM_CNTL
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111)
      );
}

void ib_vap_stream_cntl__3()
{
  //////////////////////////////////////////////////////////////////////////////
  // VAP_PROG_STREAM_CNTL
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_3
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_Z
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111)
      );
}

void ib_vap_stream_cntl__32()
{
  //////////////////////////////////////////////////////////////////////////////
  // VAP_PROG_STREAM_CNTL
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_3
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(0)
      | VAP_PROG_STREAM_CNTL__DATA_TYPE_1__FLOAT_2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_1(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_1(1)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_1(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_Z
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111) // XYZW
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_1__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_1__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_1__SELECT_FP_ZERO
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_1__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_1(0b1111) // XYZW
      );
}

void ib_vap_stream_cntl__42()
{
  //////////////////////////////////////////////////////////////////////////////
  // VAP_PROG_STREAM_CNTL
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_4
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(0)
      | VAP_PROG_STREAM_CNTL__DATA_TYPE_1__FLOAT_2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_1(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_1(1)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_1(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_Z
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_W
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111) // XYZW
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_1__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_1__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_1__SELECT_FP_ZERO
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_1__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_1(0b1111) // XYZW
      );
}

void ib_vap_stream_cntl__323()
{
  //////////////////////////////////////////////////////////////////////////////
  // VAP_PROG_STREAM_CNTL
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_3
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(0)
      | VAP_PROG_STREAM_CNTL__DATA_TYPE_1__FLOAT_2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_1(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_1(1)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_1(0)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_Z
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111) // XYZW
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_1__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_1__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_1__SELECT_FP_ZERO
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_1__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_1(0b1111) // XYZW
      );

  T0V(VAP_PROG_STREAM_CNTL_1
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_3
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(2)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_1
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_Z
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111) // XYZW
      );
}
