#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct shader_offset {
  int start; // in bytes
  int size; // in bytes
  void * buf;
};

void load_pvs_shaders(struct shader_offset * offsets, int offsets_length);

void load_us_shaders(struct shader_offset * offsets, int offsets_length);

struct shader_offset * load_shaders(const char ** paths, int paths_length);

struct shaders {
  struct shader_offset * vertex;
  struct shader_offset * fragment;
  int vertex_length;
  int fragment_length;
};

#ifdef __cplusplus
}
#endif
