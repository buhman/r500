#include <assert.h>
#include <stdlib.h>

#include "../file.h"

#include "3d_registers.h"
#include "3d_registers_undocumented.h"
#include "3d_registers_bits.h"

#include "command_processor.h"
#include "indirect_buffer.h"
#include "shader.h"

void load_pvs_shaders(struct shader_offset * offsets, int offsets_length)
{
  struct shader_offset * last_offset = &offsets[offsets_length - 1];
  int offset = last_offset->start + last_offset->size;
  int instruction_dwords = offset / 4;

  T0V(VAP_PVS_VECTOR_INDX_REG
      , VAP_PVS_VECTOR_INDX_REG__OCTWORD_OFFSET(0)
      );

  T0_ONE_REG(VAP_PVS_VECTOR_DATA_REG_128, instruction_dwords - 1);
  for (int i = 0; i < offsets_length; i++) {
    for (int j = 0; j < offsets[i].size / 4; j++) {
      TU(((uint32_t*)offsets[i].buf)[j]);
    }
  }
}

void load_us_shaders(struct shader_offset * offsets, int offsets_length)
{
  struct shader_offset * last_offset = &offsets[offsets_length - 1];
  int offset = last_offset->start + last_offset->size;
  int instruction_dwords = offset / 4;

  T0V(GA_US_VECTOR_INDEX
      , GA_US_VECTOR_INDEX__INDEX(0) // starting from index 0
      | GA_US_VECTOR_INDEX__TYPE(0)  // load instructions
      );

  T0_ONE_REG(GA_US_VECTOR_DATA, instruction_dwords - 1);
  for (int i = 0; i < offsets_length; i++) {
    for (int j = 0; j < offsets[i].size / 4; j++) {
      TU(((uint32_t*)offsets[i].buf)[j]);
    }
  }
}

struct shader_offset * load_shaders(const char ** paths, int paths_length)
{
  int offset = 0;

  struct shader_offset * offsets = malloc((sizeof (struct shader_offset)) * paths_length);

  for (int i = 0; i < paths_length; i++) {
    const char * path = paths[i];

    int size;
    void * buf = file_read(path, &size);
    assert(buf != NULL);
    assert(size % 4 == 0);
    assert(offset + size <= 1024);

    offsets[i].start = offset;
    offsets[i].size = size;
    offsets[i].buf = buf;

    offset += size;
  }

  return offsets;
}
