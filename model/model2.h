struct index_ptn {
  uint16_t position;
  uint16_t texture;
  uint16_t normal;
};

typedef index_ptn triangle_t[3];

struct object {
  const triangle_t * triangle;
  const int triangle_count;
};

struct model {
  const vec3 * position;
  const vec2 * texture;
  const vec3 * normal;
  const struct object ** object;
  const int object_count;
};
