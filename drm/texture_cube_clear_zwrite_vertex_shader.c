#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>
#include <libdrm/radeon_drm.h>

#include "3d_registers.h"
#include "3d_registers_undocumented.h"
#include "3d_registers_bits.h"
#include "command_processor.h"

#define PI (3.14159274101257324219f)
#define PI_2 (PI * 2.0f)
#define I_PI_2 (1.0f / (PI_2))

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

static void * read_file(const char * filename)
{
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "open(%s): %s\n", filename, strerror(errno));
    return NULL;
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size == (off_t)-1) {
    fprintf(stderr, "lseek(%s, SEEK_END): %s\n", filename, strerror(errno));
    return NULL;
  }

  off_t start = lseek(fd, 0, SEEK_SET);
  if (start == (off_t)-1) {
    fprintf(stderr, "lseek(%s, SEEK_SET): %s\n", filename, strerror(errno));
    return NULL;
  }

  void * buf = malloc(size+1);

  ssize_t read_size = read(fd, buf, size);
  if (read_size == -1) {
    fprintf(stderr, "read(%s): %s\n", filename, strerror(errno));
    return NULL;
  }
  ((char*)buf)[read_size] = 0;

  close(fd);

  return buf;
}

typedef struct {
  float x;
  float y;
  float z;
} vec3;

typedef struct {
  float x;
  float y;
} vec2;

typedef struct {
  int p;
  int t;
} face;

const vec3 position[] = {
  { 1.0,  1.0, -1.0},
  { 1.0, -1.0, -1.0},
  { 1.0,  1.0,  1.0},
  { 1.0, -1.0,  1.0},
  {-1.0,  1.0, -1.0},
  {-1.0, -1.0, -1.0},
  {-1.0,  1.0,  1.0},
  {-1.0, -1.0,  1.0},
};

const vec2 texture[] = {
  {1.0, 0.0},
  {0.0, 1.0},
  {0.0, 0.0},
  {1.0, 1.0},
};

const face faces[] = {
  {5,1}, {3,2}, {1,3},
  {3,1}, {8,2}, {4,3},
  {7,1}, {6,2}, {8,3},
  {2,1}, {8,2}, {6,3},
  {1,1}, {4,2}, {2,3},
  {5,1}, {2,2}, {6,3},
  {5,1}, {7,4}, {3,2},
  {3,1}, {7,4}, {8,2},
  {7,1}, {5,4}, {6,2},
  {2,1}, {4,4}, {8,2},
  {1,1}, {3,4}, {4,2},
  {5,1}, {1,4}, {2,2},
};
static const int faces_length = (sizeof (faces)) / (sizeof (faces[0]));

static const uint32_t fragment_shader[] = {
#include "../shader_examples/mesa/texture_cube.fs.txt"
  // clear shader
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

  , US_ALU_RGB_INST__RED_SWIZ_A__ZERO
  | US_ALU_RGB_INST__GREEN_SWIZ_A__ZERO
  | US_ALU_RGB_INST__BLUE_SWIZ_A__ZERO
  | US_ALU_RGB_INST__RED_SWIZ_B__ZERO
  | US_ALU_RGB_INST__GREEN_SWIZ_B__ZERO
  | US_ALU_RGB_INST__BLUE_SWIZ_B__ZERO
  | US_ALU_RGB_INST__OMOD(7)
  | US_ALU_RGB_INST__TARGET__A

  , US_ALU_ALPHA_INST__ALPHA_OP__OP_MAX
  | US_ALU_ALPHA_INST__ALPHA_SWIZ_A__ONE
  | US_ALU_ALPHA_INST__ALPHA_SWIZ_B__ONE
  | US_ALU_ALPHA_INST__OMOD(7)
  | US_ALU_ALPHA_INST__TARGET__A

  , US_ALU_RGBA_INST__RGB_OP__OP_MAX
};
static const int fragment_shader_length = (sizeof (fragment_shader)) / (sizeof (fragment_shader[0]));
static const int fragment_shader_instructions = (fragment_shader_length / 6) - 1;

static const uint32_t vertex_shader[] = {
  //#include "../shader_examples/mesa/texture_cube.vs.txt"
  #include "cube_rotate.vs.inc"
  #include "clear_nop.vs.inc"
};
static const int vertex_shader_length = (sizeof (vertex_shader)) / (sizeof (vertex_shader[0]));
static const int vertex_shader_instructions = (vertex_shader_length / 4) - 1;

union u32_f32 {
  uint32_t u32;
  float f32;
};

static union u32_f32 ib[16384];

int _3d_clear(int ix)
{
  //////////////////////////////////////////////////////////////////////////////
  // ZB
  //////////////////////////////////////////////////////////////////////////////

  T0V(ZB_CNTL
      , ZB_CNTL__Z_ENABLE__ENABLED // 1
      | ZB_CNTL__ZWRITEENABLE__ENABLE // 1
      );
  T0V(ZB_ZSTENCILCNTL
      , ZB_ZSTENCILCNTL__ZFUNC__ALWAYS // greater than
      );

  T0V(ZB_FORMAT
      , ZB_FORMAT__DEPTHFORMAT(2) // 24-bit integer Z, 8 bit stencil
      );

  T0V(ZB_DEPTHOFFSET, 0);
  T3(_NOP, 0);
  ib[ix++].u32 = 1 * 4; // index into relocs array

  T0V(ZB_DEPTHPITCH
      , ZB_DEPTHPITCH__DEPTHPITCH(1600 >> 2)
      | ZB_DEPTHPITCH__DEPTHMACROTILE(1)
      | ZB_DEPTHPITCH__DEPTHMICROTILE(1)
      );
  T3(_NOP, 0);
  ib[ix++].u32 = 1 * 4; // index into relocs array

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
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_ENABLE, 0x00000000);

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_CODE_CNTL_0
      , VAP_PVS_CODE_CNTL_0__PVS_FIRST_INST(vertex_shader_instructions)
      | VAP_PVS_CODE_CNTL_0__PVS_XYZW_VALID_INST(vertex_shader_instructions)
      | VAP_PVS_CODE_CNTL_0__PVS_LAST_INST(vertex_shader_instructions)
      );
  T0V(VAP_PVS_CODE_CNTL_1
      , VAP_PVS_CODE_CNTL_1__PVS_LAST_VTX_SRC_INST(vertex_shader_instructions)
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__CLIP_DISABLE(1)
      );

  T0V(VAP_VTE_CNTL
      , VAP_VTE_CNTL__VTX_XY_FMT(1)
      | VAP_VTE_CNTL__VTX_Z_FMT(1)
      );

  T0V(VAP_CNTL_STATUS, VAP_CNTL_STATUS__PVS_BYPASS(0));

  T0V(VAP_PROG_STREAM_CNTL_0
      , VAP_PROG_STREAM_CNTL__DATA_TYPE_0__FLOAT_2
      | VAP_PROG_STREAM_CNTL__SKIP_DWORDS_0(0)
      | VAP_PROG_STREAM_CNTL__DST_VEC_LOC_0(0)
      | VAP_PROG_STREAM_CNTL__LAST_VEC_0(1)
      );
  T0V(VAP_PROG_STREAM_CNTL_EXT_0
      , VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_X_0__SELECT_X
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Y_0__SELECT_Y
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_Z_0__SELECT_FP_ZERO
      | VAP_PROG_STREAM_CNTL_EXT__SWIZZLE_SELECT_W_0__SELECT_FP_ONE
      | VAP_PROG_STREAM_CNTL_EXT__WRITE_ENA_0(0b1111)
      );

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(2)
      );

  T0V(VAP_INDEX_OFFSET, 0x00000000);

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , 0x0);

  //////////////////////////////////////////////////////////////////////////////
  // GA_US
  //////////////////////////////////////////////////////////////////////////////

  T0V(US_CODE_RANGE
      , US_CODE_RANGE__CODE_ADDR(fragment_shader_instructions)
      | US_CODE_RANGE__CODE_SIZE(0)
      );
  T0V(US_CODE_OFFSET
      , US_CODE_OFFSET__OFFSET_ADDR(fragment_shader_instructions)
      );
  T0V(US_CODE_ADDR
      , US_CODE_ADDR__START_ADDR(0)
      | US_CODE_ADDR__END_ADDR(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

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

  return ix;
}

int _3d_cube(int ix, float theta)
{
  printf("faces length %d\n", faces_length);

  //////////////////////////////////////////////////////////////////////////////
  // ZB
  //////////////////////////////////////////////////////////////////////////////

  T0V(ZB_CNTL
      , ZB_CNTL__Z_ENABLE__ENABLED // 1
      | ZB_CNTL__ZWRITEENABLE__ENABLE // 1
      );
  T0V(ZB_ZSTENCILCNTL
      , ZB_ZSTENCILCNTL__ZFUNC(5) // greater than
      );

  T0V(ZB_FORMAT
      , ZB_FORMAT__DEPTHFORMAT(2) // 24-bit integer Z, 8 bit stencil
      );

  T0V(ZB_DEPTHOFFSET, 0);
  T3(_NOP, 0);
  ib[ix++].u32 = 1 * 4; // index into relocs array

  T0V(ZB_DEPTHPITCH
      , ZB_DEPTHPITCH__DEPTHPITCH(1600 >> 2)
      | ZB_DEPTHPITCH__DEPTHMACROTILE(1)
      | ZB_DEPTHPITCH__DEPTHMICROTILE(1)
      );
  T3(_NOP, 0);
  ib[ix++].u32 = 1 * 4; // index into relocs array

  //////////////////////////////////////////////////////////////////////////////
  // RS
  //////////////////////////////////////////////////////////////////////////////

  T0V(RS_IP_0
      , RS_IP__TEX_PTR_S(0)
      | RS_IP__TEX_PTR_T(1)
      | RS_IP__TEX_PTR_R(2)
      | RS_IP__TEX_PTR_Q(3)
      | RS_IP__COL_PTR(0)
      | RS_IP__COL_FMT(0)
      | RS_IP__OFFSET_EN(0)
      );
  T0V(RS_COUNT
      , RS_COUNT__IT_COUNT(4)
      | RS_COUNT__IC_COUNT(0)
      | RS_COUNT__W_ADDR(0)
      | RS_COUNT__HIRES_EN(1)
      );
  T0V(RS_INST_COUNT, 0x00000000);
  T0V(RS_INST_0
      , RS_INST__TEX_ID(0)
      | RS_INST__TEX_CN(1)
      | RS_INST__TEX_ADDR(0)
      );

  //////////////////////////////////////////////////////////////////////////////
  // TX
  //////////////////////////////////////////////////////////////////////////////

  T0V(TX_INVALTAGS, 0x00000000);

  T0V(TX_ENABLE
      , TX_ENABLE__TEX_0_ENABLE__ENABLE);
  T0V(TX_FILTER0_0
      , TX_FILTER0__MAG_FILTER__LINEAR
      | TX_FILTER0__MIN_FILTER__LINEAR
      );
  T0V(TX_FILTER1_0
      , TX_FILTER1__LOD_BIAS(1)
      );
  T0V(TX_BORDER_COLOR_0, 0);
  T0V(TX_FORMAT0_0
      , TX_FORMAT0__TXWIDTH(1024 - 1)
      | TX_FORMAT0__TXHEIGHT(1024 - 1)
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
      //, TX_OFFSET__MACRO_TILE(1)
      //| TX_OFFSET__MICRO_TILE(1)
      , 0
      );

  T3(_NOP, 0);
  ib[ix++].u32 = 2 * 4; // index into relocs array

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_PVS_CONST_CNTL
      , VAP_PVS_CONST_CNTL__PVS_CONST_BASE_OFFSET(0)
      | VAP_PVS_CONST_CNTL__PVS_MAX_CONST_ADDR(1)
      );

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1024)
      );

  float theta1 = theta;
  //float theta2 = 3.14f * theta;
  float theta2 = theta;
  float consts[] = {
    I_PI_2, 0.5f, PI_2, -PI,
    theta1, theta2, 0.2f, 0.5f,
  };
  int consts_length = (sizeof (consts)) / (sizeof (consts[0]));
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, (consts_length - 1));
  for (int i = 0; i < consts_length; i++)
    ib[ix++].f32 = consts[i];

  T0V(VAP_PVS_CODE_CNTL_0
      , VAP_PVS_CODE_CNTL_0__PVS_FIRST_INST(0)
      | VAP_PVS_CODE_CNTL_0__PVS_XYZW_VALID_INST((vertex_shader_instructions - 1))
      | VAP_PVS_CODE_CNTL_0__PVS_LAST_INST((vertex_shader_instructions - 1))
      );
  T0V(VAP_PVS_CODE_CNTL_1
      , VAP_PVS_CODE_CNTL_1__PVS_LAST_VTX_SRC_INST((vertex_shader_instructions - 1))
      );

  //////////////////////////////////////////////////////////////////////////////
  // VAP
  //////////////////////////////////////////////////////////////////////////////

  T0V(VAP_CLIP_CNTL
      , VAP_CLIP_CNTL__PS_UCP_MODE(3)
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

  T0V(VAP_CNTL_STATUS, 0);

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

  T0V(VAP_VTX_SIZE
      , VAP_VTX_SIZE__DWORDS_PER_VTX(5)
      );

  T0V(VAP_INDEX_OFFSET, 0x00000000);

  T0V(VAP_VF_MAX_VTX_INDX
      , VAP_VF_MAX_VTX_INDX__MAX_INDX(faces_length - 1)
      );
  T0V(VAP_VF_MIN_VTX_INDX
      , VAP_VF_MIN_VTX_INDX__MIN_INDX(0)
      );

  T0V(VAP_OUT_VTX_FMT_0
      , VAP_OUT_VTX_FMT_0__VTX_POS_PRESENT(1));
  T0V(VAP_OUT_VTX_FMT_1
      , VAP_OUT_VTX_FMT_1__TEX_0_COMP_CNT(4));

  //////////////////////////////////////////////////////////////////////////////
  // GA_US
  //////////////////////////////////////////////////////////////////////////////

  T0V(US_CODE_RANGE
      , US_CODE_RANGE__CODE_ADDR(0)
      | US_CODE_RANGE__CODE_SIZE(fragment_shader_instructions - 1)
      );
  T0V(US_CODE_OFFSET
      , US_CODE_OFFSET__OFFSET_ADDR(0)
      );
  T0V(US_CODE_ADDR
      , US_CODE_ADDR__START_ADDR(0)
      | US_CODE_ADDR__END_ADDR(fragment_shader_instructions - 1)
      );

  //////////////////////////////////////////////////////////////////////////////
  // 3D_DRAW
  //////////////////////////////////////////////////////////////////////////////

  T3(_3D_DRAW_IMMD_2, (1 + faces_length * 5) - 1);
  ib[ix++].u32
    = VAP_VF_CNTL__PRIM_TYPE(4)
    | VAP_VF_CNTL__PRIM_WALK(3)
    | VAP_VF_CNTL__INDEX_SIZE(0)
    | VAP_VF_CNTL__VTX_REUSE_DIS(0)
    | VAP_VF_CNTL__DUAL_INDEX_MODE(0)
    | VAP_VF_CNTL__USE_ALT_NUM_VERTS(0)
    | VAP_VF_CNTL__NUM_VERTICES(faces_length)
    ;
  for (int i = 0; i < faces_length; i++) {
    vec3 p = position[faces[i].p - 1];
    vec2 t = texture[faces[i].t - 1];

    ib[ix++].f32 = p.x;
    ib[ix++].f32 = p.y;
    ib[ix++].f32 = p.z;
    ib[ix++].f32 = t.x;
    ib[ix++].f32 = t.y;
  }

  return ix;
}

int indirect_buffer(float theta)
{
  int ix = 0;

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

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(1536));
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

  T0Vf(VAP_VPORT_XSCALE,   600.0f);
  T0Vf(VAP_VPORT_XOFFSET,  800.0f);
  T0Vf(VAP_VPORT_YSCALE,  -600.0f);
  T0Vf(VAP_VPORT_YOFFSET,  600.0f);
  T0Vf(VAP_VPORT_ZSCALE,     0.5f);
  T0Vf(VAP_VPORT_ZOFFSET,    0.5f);

  T0V(VAP_VSM_VTX_ASSM
      , 0x00000401); // undocumented

  //////////////////////////////////////////////////////////////////////////////
  // VAP_PVS
  //////////////////////////////////////////////////////////////////////////////

  printf("vs length %d\n", vertex_shader_length);
  assert(vertex_shader_length % 4 == 0);
  printf("vs instructions %d\n", vertex_shader_instructions);

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(0)
      );
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, vertex_shader_length - 1);
  for (int i = 0; i < vertex_shader_length; i++) {
    ib[ix++].u32 = vertex_shader[i];
  }

  //////////////////////////////////////////////////////////////////////////////
  // GA_US
  //////////////////////////////////////////////////////////////////////////////

  printf("fs length %d\n", fragment_shader_length);
  assert(fragment_shader_length % 6 == 0);
  printf("fs instructions %d\n", fragment_shader_instructions);

  T0V(GA_US_VECTOR_INDEX, 0x00000000);
  T0_ONE_REG(GA_US_VECTOR_DATA, fragment_shader_length - 1);
  for (int i = 0; i < fragment_shader_length; i++) {
    ib[ix++].u32 = fragment_shader[i];
  }

  //////////////////////////////////////////////////////////////////////////////
  // DRAW
  //////////////////////////////////////////////////////////////////////////////

  ix = _3d_clear(ix);
  ix = _3d_cube(ix, theta);

  //////////////////////////////////////////////////////////////////////////////
  // padding
  //////////////////////////////////////////////////////////////////////////////

  while ((ix % 8) != 0) {
    ib[ix++].u32 = 0x80000000;
  }

  return ix;
}

int create_colorbuffer(int fd, int colorbuffer_size)
{
  int ret;

  struct drm_radeon_gem_create args = {
    .size = colorbuffer_size,
    .alignment = 4096,
    .handle = 0,
    .initial_domain = 4, // RADEON_GEM_DOMAIN_VRAM
    .flags = 4
  };

  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE, &args, (sizeof (struct drm_radeon_gem_create)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
  }
  assert(args.handle != 0);

  struct drm_radeon_gem_mmap mmap_args = {
    .handle = args.handle,
    .offset = 0,
    .size = colorbuffer_size,
  };
  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_MMAP, &mmap_args, (sizeof (struct drm_radeon_gem_mmap)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_MMAP)");
  }

  void * ptr = mmap(0,
                    colorbuffer_size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    mmap_args.addr_ptr);
  assert(ptr != MAP_FAILED);

  // clear colorbuffer
  for (int i = 0; i < colorbuffer_size / 4; i++) {
    ((uint32_t*)ptr)[i] = 0x00000000;
  }
  asm volatile ("" ::: "memory");

  munmap(ptr, colorbuffer_size);

  return args.handle;
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
  int zbuffer_handle;
  int texturebuffer_handle;
  int flush_handle;

  // colorbuffer
  colorbuffer_handle[0] = create_colorbuffer(fd, colorbuffer_size);
  colorbuffer_handle[1] = create_colorbuffer(fd, colorbuffer_size);
  zbuffer_handle = create_colorbuffer(fd, colorbuffer_size);

  // texture
  {
    const int texture_size = 1024 * 1024 * 4;

    struct drm_radeon_gem_create args = {
      .size = texture_size,
      .alignment = 4096,
      .handle = 0,
      .initial_domain = 4, // RADEON_GEM_DOMAIN_VRAM
      .flags = 4
    };

    ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_CREATE, &args, (sizeof (struct drm_radeon_gem_create)));
    if (ret != 0) {
      perror("drmCommandWriteRead(DRM_RADEON_GEM_CREATE)");
    }
    assert(args.handle != 0);

    texturebuffer_handle = args.handle;

    struct drm_radeon_gem_mmap mmap_args = {
      .handle = texturebuffer_handle,
      .offset = 0,
      .size = texture_size,
    };
    ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_MMAP, &mmap_args, (sizeof (struct drm_radeon_gem_mmap)));
    if (ret != 0) {
      perror("drmCommandWriteRead(DRM_RADEON_GEM_MMAP)");
    }

    void * texturebuffer_ptr = mmap(0, mmap_args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
                                    fd, mmap_args.addr_ptr);
    assert(texturebuffer_ptr != MAP_FAILED);

    // copy texture
    void * texture_buf = read_file("../texture/butterfly_1024x1024_argb8888.data");
    assert(texture_buf != NULL);

    for (int i = 0; i < texture_size / 4; i++) {
      ((uint32_t*)texturebuffer_ptr)[i] = ((uint32_t*)texture_buf)[i];
    }
    asm volatile ("" ::: "memory");
    free(texture_buf);
    munmap(texturebuffer_ptr, texture_size);
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

  fprintf(stderr, "colorbuffer handle[0] %d\n", colorbuffer_handle[0]);
  fprintf(stderr, "colorbuffer handle[1] %d\n", colorbuffer_handle[1]);

  uint32_t flags[2] = {
    5, // RADEON_CS_KEEP_TILING_FLAGS | RADEON_CS_END_OF_FRAME
    0, // RADEON_CS_RING_GFX
  };

  int ib_dwords = indirect_buffer(0);
  //int ib_dwords = (sizeof (ib2)) / (sizeof (ib2[0]));

  int colorbuffer_ix = 0;
  float theta = 0;

  while (true) {
    struct drm_radeon_cs_reloc relocs[] = {
      {
        .handle = colorbuffer_handle[colorbuffer_ix],
        .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
        .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
        .flags = 8,
      },
      {
        .handle = zbuffer_handle,
        .read_domains = 4, // RADEON_GEM_DOMAIN_VRAM
        .write_domain = 4, // RADEON_GEM_DOMAIN_VRAM
        .flags = 8,
      },
      {
        .handle = texturebuffer_handle,
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

    uint32_t d1crtc_double_buffer_control = rreg(rmmio, D1CRTC_DOUBLE_BUFFER_CONTROL);
    printf("D1CRTC_DOUBLE_BUFFER_CONTROL: %08x\n", d1crtc_double_buffer_control);
    assert(d1crtc_double_buffer_control == (1 << 8));

    // addresses were retrieved from /sys/kernel/debug/radeon_vram_mm
    //
    // This assumes GEM buffer allocation always starts from the lowest
    // unallocated address.
    const uint32_t colorbuffer_addresses[2] = {
      0x813000,
      0xf66000,
    };

    wreg(rmmio, D1GRPH_PRIMARY_SURFACE_ADDRESS, colorbuffer_addresses[colorbuffer_ix]);
    while ((rreg(rmmio, D1GRPH_UPDATE) & D1GRPH_UPDATE__D1GRPH_SURFACE_UPDATE_PENDING) != 0);

    // next state
    theta += 0.01f;
    colorbuffer_ix = (colorbuffer_ix + 1) & 1;

    // next indirect buffer
    ib_dwords = indirect_buffer(theta);
  }

  /*
  int out_fd = open("colorbuffer.data", O_RDWR|O_CREAT);
  assert(out_fd >= 0);
  ssize_t write_length = write(out_fd, colorbuffer_ptr, colorbuffer_size);
  assert(write_length == colorbuffer_size);
  close(out_fd);

  int mm_fd = open("/sys/kernel/debug/radeon_vram_mm", O_RDONLY);
  assert(mm_fd >= 0);
  char buf[4096];
  while (true) {
    ssize_t read_length = read(mm_fd, buf, 4096);
    assert(read_length >= 0);
    write(STDOUT_FILENO, buf, read_length);
    if (read_length < 4096) {
      break;
    }
  }
  close(mm_fd);
  */

  close(fd);
}
