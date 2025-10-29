#pragma once

typedef struct vec3 vertex_position;
typedef struct vec2 vertex_texture;
typedef struct vec3 vertex_normal;

struct index_ptn {
  uint16_t position;
  uint16_t texture;
  uint16_t normal;
};

union triangle {
  struct {
    struct index_ptn a;
    struct index_ptn b;
    struct index_ptn c;
  };
  struct index_ptn v[3];
};

union quadrilateral {
  struct {
    struct index_ptn a;
    struct index_ptn b;
    struct index_ptn c;
    struct index_ptn d;
  };
  struct index_ptn v[4];
};

union line {
  struct {
    int a;
    int b;
  };
  int v[2];
};

struct object {
  const union triangle * triangle;
  const union quadrilateral * quadrilateral;
  const union line * line;
  const int triangle_count;
  const int quadrilateral_count;
  const int line_count;
  const int material;
};

struct model {
  const vertex_position * position;
  const vertex_texture * texture;
  const vertex_normal * normal;
  const struct object ** object;
  const int object_count;
};
