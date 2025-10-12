#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <xf86drm.h>
#include <libdrm/radeon_drm.h>

#include "3d_registers.h"
#include "3d_registers_undocumented.h"

static uint32_t ib[16384];

#define TYPE_0_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_0_ONE_REG (1 << 15)
#define TYPE_0_BASE_INDEX(i) (((i) & 0x1fff) << 0)

#define TYPE_3_COUNT(c) (((c) & 0x3fff) << 16)
#define TYPE_3_OPCODE(o) (((o) & 0xff) << 8)

#define T0(address, count)                                              \
  do {                                                                  \
    ib[ix++] = TYPE_0_COUNT(count) | TYPE_0_BASE_INDEX(address >> 2);   \
  } while (0);

#define T0_ONE_REG(address, count)                                      \
  do {                                                                  \
    ib[ix++] = TYPE_0_COUNT(count) | TYPE_0_ONE_REG | TYPE_0_BASE_INDEX(address >> 2);   \
  } while (0);

#define T0V(address, value)                                             \
  do {                                                                  \
    ib[ix++] = TYPE_0_COUNT(0) | TYPE_0_BASE_INDEX(address >> 2);       \
    ib[ix++] = value;                                                   \
  } while (0);

#define T3(opcode, count)                                               \
  do {                                                                  \
    ib[ix++] = (0b11 << 30) | TYPE_3_COUNT(count) | TYPE_3_OPCODE(opcode);  \
  } while (0);

int indirect_buffer()
{
  int ix = 0;

  T0V(SC_SCISSOR0, 0x0);
  T0V(SC_SCISSOR1, ((1200 - 1) << 13) | ((1600 - 1) << 0));

  T0V(RB3D_DSTCACHE_CTLSTAT, 0x0000000a);

  T0V(ZB_ZCACHE_CTLSTAT, 0x00000003);

  T0V(WAIT_UNTIL, 0x00020000);

  T0V(GB_AA_CONFIG, 0x00000000);

  T0V(RB3D_AARESOLVE_CTL, 0x00000000);

  T0V(RB3D_CCTL, 0x00004000);

  T0V(RB3D_COLOROFFSET0, 0x00000000);
  ib[ix++] = 0xc0001000;
  ib[ix++] = 0x0;

  T0V(RB3D_COLORPITCH0, (6 << 21) | (1600 << 0));
  ib[ix++] = 0xc0001000;
  ib[ix++] = 0x0;

  T0V(ZB_BW_CNTL, 0x00000000);
  T0V(ZB_DEPTHCLEARVALUE, 0x00000000);
  T0V(SC_HYPERZ_EN, 0x00000000);
  T0V(GB_Z_PEQ_CONFIG, 0x00000000);
  T0V(ZB_ZTOP, 0x00000001);
  T0V(FG_ALPHA_FUNC, 0x00000000);
  T0V(ZB_CNTL, 0x00000000);
  T0V(ZB_ZSTENCILCNTL, 0x00000000);
  T0V(ZB_STENCILREFMASK, 0x00000000);
  T0V(ZB_STENCILREFMASK_BF, 0x00000000);

  T0V(FG_ALPHA_VALUE, 0x00000000);
  T0V(RB3D_ROPCNTL, 0x00000000);
  T0V(RB3D_BLENDCNTL, 0x00000000);
  T0V(RB3D_ABLENDCNTL, 0x00000000);
  T0V(RB3D_COLOR_CHANNEL_MASK, 0x0000000f);
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
  T0V(SU_DEPTH_SCALE, 0x4b7fffff);
  T0V(SU_DEPTH_OFFSET, 0x00000000);
  T0V(SC_EDGERULE, 0x2da49525);
  T0V(RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x01010101);
  T0V(RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xfefefefe);
  T0V(GA_COLOR_CONTROL_PS3, 0x00000000);
  T0V(SU_TEX_WRAP_PS3, 0x00000000);
  T0V(VAP_VPORT_XSCALE, 0x44480000);
  T0V(VAP_VPORT_XOFFSET, 0x44480000);
  T0V(VAP_VPORT_YSCALE, 0xc4160000);
  T0V(VAP_VPORT_YOFFSET, 0x44160000);
  T0V(VAP_VPORT_ZSCALE, 0x3f000000);
  T0V(VAP_VPORT_ZOFFSET, 0x3f000000);
  T0V(VAP_VTE_CNTL, 0x0000043f);
  T0V(VAP_PVS_STATE_FLUSH_REG, 0x00000000);
  T0V(VAP_PVS_VTX_TIMEOUT_REG, 0x0000ffff);
  T0V(VAP_GB_VERT_CLIP_ADJ, 0x3f800000);
  T0V(VAP_GB_VERT_DISC_ADJ, 0x3f800000);
  T0V(VAP_GB_HORZ_CLIP_ADJ, 0x3f800000);
  T0V(VAP_GB_HORZ_DISC_ADJ, 0x3f800000);
  T0V(VAP_PSC_SGN_NORM_CNTL, 0xaaaaaaaa);
  T0V(VAP_TEX_TO_COLOR_CNTL, 0x00000000);
  T0V(VAP_PROG_STREAM_CNTL_0, 0x00002002);
  T0V(VAP_PROG_STREAM_CNTL_EXT_0, 0x0000fa88);
  T0V(VAP_PVS_CODE_CNTL_0, 0x00000000);
  T0V(VAP_PVS_CODE_CNTL_1, 0x00000000);
  T0V(VAP_PVS_VECTOR_INDX_REG, 0x00000000);

  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, 3);
  ib[ix++] = 0x00f00203;
  ib[ix++] = 0x00d10001;
  ib[ix++] = 0x01248001;
  ib[ix++] = 0x01248001;

  T0V(VAP_CNTL, 0x00b0055a);
  T0V(VAP_PVS_FLOW_CNTL_OPC, 0x00000000);

  T0(VAP_PVS_FLOW_CNTL_ADDRS_LW_0, 31);
  for (int i = 0; i < 32; i++)
    ib[ix++] = 0x00000000;

  T0(VAP_PVS_FLOW_CNTL_LOOP_INDEX_0, 15);
  for (int i = 0; i < 16; i++)
    ib[ix++] = 0x00000000;

  T0V(VAP_PVS_VECTOR_INDX_REG, 0x00000600);
  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, 23);
  for (int i = 0; i < 24; i++)
    ib[ix++] = 0x00000000;

  T0V(VAP_VTX_STATE_CNTL, 0x00005555);
  T0V(VAP_VSM_VTX_ASSM, 0x00000001);
  T0V(VAP_OUT_VTX_FMT_0, 0x00000001);
  T0V(VAP_OUT_VTX_FMT_1, 0x00000000);
  T0V(GB_ENABLE, 0x00000000);
  T0V(RS_IP_0, 0x30000000);
  T0V(RS_COUNT, 0x00040080);
  T0V(RS_INST_COUNT, 0x00000000);
  T0V(RS_INST_0, 0x00000000);
  T0V(VAP_CNTL_STATUS, 0x00000000);
  T0V(VAP_CLIP_CNTL, 0x0000c000);
  T0V(GA_POINT_SIZE, 0x00060006);
  T0V(GA_POINT_MINMAX, 0x00060006);
  T0V(GA_LINE_CNTL, 0x00020006);
  T0V(SU_POLY_OFFSET_ENABLE, 0x00000000);
  T0V(SU_CULL_MODE, 0x00000000);
  T0V(GA_LINE_STIPPLE_CONFIG, 0x00000000);
  T0V(GA_LINE_STIPPLE_VALUE, 0x00000000);
  T0V(GA_POLY_MODE, 0x00000000);
  T0V(GA_ROUND_MODE, 0x00000031);
  T0V(SC_CLIP_RULE, 0x0000ffff);
  T0V(GA_POINT_S0, 0x00000000);
  T0V(GA_POINT_T0, 0x3f800000);
  T0V(GA_POINT_S1, 0x3f800000);
  T0V(GA_POINT_T1, 0x00000000);
  T0V(US_OUT_FMT_0, 0x00001b00);
  T0V(US_OUT_FMT_1, 0x0000000f);
  T0V(US_OUT_FMT_2, 0x0000000f);
  T0V(US_OUT_FMT_3, 0x0000000f);
  T0V(GB_MSPOS0, 0x66666666);
  T0V(GB_MSPOS1, 0x06666666);
  T0V(US_CONFIG, 0x00000002);
  T0V(US_PIXSIZE, 0x00000001);
  T0V(US_FC_CTRL, 0x00000000);
  T0V(US_CODE_RANGE, 0x00000000);
  T0V(US_CODE_OFFSET, 0x00000000);
  T0V(US_CODE_ADDR, 0x00000000);

  T0V(GA_US_VECTOR_INDEX, 0x00000000);

  T0_ONE_REG(GA_US_VECTOR_DATA, 5);
  ib[ix++] = 0x00078005;
  ib[ix++] = 0x08020080;
  ib[ix++] = 0x08020080;
  ib[ix++] = 0x1c9b04d8;
  ib[ix++] = 0x1c810003;
  ib[ix++] = 0x00000005;

  T0V(FG_DEPTH_SRC, 0x00000000);
  T0V(US_W_FMT, 0x00000000);
  T0V(VAP_PVS_CONST_CNTL, 0x00000000);
  T0V(TX_INVALTAGS, 0x00000000);
  T0V(TX_ENABLE, 0x00000000);
  T0V(VAP_INDEX_OFFSET, 0x00000000);
  T0V(GA_COLOR_CONTROL, 0x0003aaaa);
  T0V(VAP_VF_MAX_VTX_INDX, 0x00000002);
  T0V(VAP_VF_MIN_VTX_INDX, 0x00000000);
  T0V(VAP_VTX_SIZE, 0x00000003);

  T3(0x35, 9);
  ib[ix++] = 0x00030034;
  ib[ix++] = 0x3f000000;
  ib[ix++] = 0xbf800000; //0xbf000000;
  ib[ix++] = 0x00000000;
  ib[ix++] = 0xbf800000; //0xbf000000
  ib[ix++] = 0xbf800000; //0xbf000000
  ib[ix++] = 0x00000000;
  ib[ix++] = 0x00000000;
  ib[ix++] = 0x3f000000;
  ib[ix++] = 0x00000000;

  while ((ix % 8) != 0) {
    ib[ix++] = 0x80000000;
  }

  return ix;
}

int main()
{
  int ret;
  int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

  int colorbuffer_handle;
  int flush_handle;

  // colorbuffer
  {
    struct drm_radeon_gem_create args = {
      .size = 1600 * 1200 * 4,
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

    colorbuffer_handle = args.handle;
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


  fprintf(stderr, "colorbuffer handle %d\n", colorbuffer_handle);

  struct drm_radeon_cs_reloc relocs[] = {
    {
      .handle = colorbuffer_handle,
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

  int ib_dwords = indirect_buffer();
  //int ib_dwords = (sizeof (ib2)) / (sizeof (ib2[0]));

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

  struct drm_radeon_gem_wait_idle args = {
    .handle = flush_handle
  };
  while (drmCommandWrite(fd, DRM_RADEON_GEM_WAIT_IDLE, &args, (sizeof (struct drm_radeon_gem_wait_idle))) == -EBUSY);

  struct drm_radeon_gem_mmap mmap_args = {
    .handle = colorbuffer_handle,
    .offset = 0,
    .size = 1600 * 1200 * 4,
  };
  ret = drmCommandWriteRead(fd, DRM_RADEON_GEM_MMAP, &mmap_args, (sizeof (struct drm_radeon_gem_mmap)));
  if (ret != 0) {
    perror("drmCommandWriteRead(DRM_RADEON_GEM_MMAP)");
  }

  void * ptr;
  ptr = mmap(0, mmap_args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
             fd, mmap_args.addr_ptr);

  int out_fd = open("colorbuffer.data", O_RDWR|O_CREAT);
  assert(out_fd >= 0);
  ssize_t write_length = write(out_fd, ptr, mmap_args.size);
  assert(write_length == mmap_args.size);
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

  munmap(ptr, mmap_args.size);

  close(fd);
}
