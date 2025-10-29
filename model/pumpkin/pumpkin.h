static const vec3 mesh_Cubemesh_position[] = {
  {1.000000, 1.000000, 1.000000},
  {1.000000, 1.000000, -1.000000},
  {1.000000, -1.000000, 1.000000},
  {1.000000, -1.000000, -1.000000},
  {-1.000000, 1.000000, 1.000000},
  {-1.000000, 1.000000, -1.000000},
  {-1.000000, -1.000000, 1.000000},
  {-1.000000, -1.000000, -1.000000},
};

static const vec2 mesh_Cubemesh_UVMap_uvmap[] = {
  {0.875000, 0.500000},
  {0.625000, 0.750000},
  {0.625000, 0.500000},
  {0.625000, 0.750000},
  {0.375000, 1.000000},
  {0.375000, 0.750000},
  {0.625000, 0.000000},
  {0.375000, 0.250000},
  {0.375000, 0.000000},
  {0.375000, 0.500000},
  {0.125000, 0.750000},
  {0.125000, 0.500000},
  {0.625000, 0.500000},
  {0.375000, 0.750000},
  {0.375000, 0.500000},
  {0.625000, 0.250000},
  {0.375000, 0.500000},
  {0.375000, 0.250000},
  {0.875000, 0.500000},
  {0.875000, 0.750000},
  {0.625000, 0.750000},
  {0.625000, 0.750000},
  {0.625000, 1.000000},
  {0.375000, 1.000000},
  {0.625000, 0.000000},
  {0.625000, 0.250000},
  {0.375000, 0.250000},
  {0.375000, 0.500000},
  {0.375000, 0.750000},
  {0.125000, 0.750000},
  {0.625000, 0.500000},
  {0.625000, 0.750000},
  {0.375000, 0.750000},
  {0.625000, 0.250000},
  {0.625000, 0.500000},
  {0.375000, 0.500000},
};

static const vec3 mesh_Cubemesh_normal[] = {
  {0.577372, 0.577339, 0.577339},
  {0.577328, 0.577361, -0.577361},
  {0.577328, -0.577361, 0.577361},
  {0.577372, -0.577339, -0.577339},
  {-0.577328, 0.577361, 0.577361},
  {-0.577372, 0.577339, -0.577339},
  {-0.577372, -0.577339, 0.577339},
  {-0.577328, -0.577361, -0.577361},
};

static const vec3 mesh_Cubemesh_polygon_normal[] = {
  {0.000000, 0.000000, 1.000000},
  {0.000000, -1.000000, 0.000000},
  {-1.000000, 0.000000, 0.000000},
  {0.000000, 0.000000, -1.000000},
  {1.000000, 0.000000, 0.000000},
  {0.000000, 1.000000, 0.000000},
  {0.000000, 0.000000, 1.000000},
  {0.000000, -1.000000, 0.000000},
  {-1.000000, 0.000000, 0.000000},
  {0.000000, 0.000000, -1.000000},
  {1.000000, 0.000000, 0.000000},
  {0.000000, 1.000000, 0.000000},
};

static const struct polygon mesh_Cubemesh_polygons[] = {
  {4, 2, 0, 0},
  {2, 7, 3, 0},
  {6, 5, 7, 0},
  {1, 7, 5, 0},
  {0, 3, 1, 0},
  {4, 1, 5, 0},
  {4, 6, 2, 0},
  {2, 6, 7, 0},
  {6, 4, 5, 0},
  {1, 3, 7, 0},
  {0, 2, 3, 0},
  {4, 0, 1, 0},
};

static const struct edge_polygon mesh_Cubemesh_edge_polygons[] = {
  {{2, 4}, {0, 6}},
  {{0, 2}, {0, 10}},
  {{0, 4}, {0, 11}},
  {{2, 7}, {1, 7}},
  {{3, 7}, {1, 9}},
  {{2, 3}, {1, 10}},
  {{5, 6}, {2, 8}},
  {{5, 7}, {2, 3}},
  {{6, 7}, {2, 7}},
  {{1, 7}, {3, 9}},
  {{1, 5}, {3, 5}},
  {{0, 3}, {4, 10}},
  {{1, 3}, {4, 9}},
  {{0, 1}, {4, 11}},
  {{1, 4}, {5, 11}},
  {{4, 5}, {5, 8}},
  {{4, 6}, {6, 8}},
  {{2, 6}, {6, 7}},
};

static const struct mesh_material mesh_Cubemesh_materials[] = {
  { // Material Untitled
    .width = 1024,
    .height = 1024,
    .texture_id = 0,
  },
};
static const vec2 * mesh_Cubemesh_uv_layers[] = {
  mesh_Cubemesh_UVMap_uvmap,
};

static const struct mesh mesh_Cubemesh = {
  .position = mesh_Cubemesh_position,
  .position_length = (sizeof (mesh_Cubemesh_position)) / (sizeof (mesh_Cubemesh_position[0])),
  .normal = mesh_Cubemesh_normal,
  .normal_length = (sizeof (mesh_Cubemesh_normal)) / (sizeof (mesh_Cubemesh_normal[0])),
  .polygon_normal = mesh_Cubemesh_polygon_normal,
  .polygon_normal_length = (sizeof (mesh_Cubemesh_polygon_normal)) / (sizeof (mesh_Cubemesh_polygon_normal[0])),
  .polygons = mesh_Cubemesh_polygons,
  .polygons_length = (sizeof (mesh_Cubemesh_polygons)) / (sizeof (mesh_Cubemesh_polygons[0])),
  .uv_layers = mesh_Cubemesh_uv_layers,
  .uv_layers_length = (sizeof (mesh_Cubemesh_uv_layers)) / (sizeof (mesh_Cubemesh_uv_layers[0])),
  .materials = mesh_Cubemesh_materials,
  .materials_length = (sizeof (mesh_Cubemesh_materials)) / (sizeof (mesh_Cubemesh_materials[0])),
  .edge_polygons = mesh_Cubemesh_edge_polygons,
  .edge_polygons_length = (sizeof (mesh_Cubemesh_edge_polygons)) / (sizeof (mesh_Cubemesh_edge_polygons[0])),
};

static const struct object objects[] = {
  { // object_Cube
    .mesh = &mesh_Cubemesh,
    .scale = {1.000000, 1.000000, 1.000000},
    .rotation = {0.000000, 0.000000, 0.000000, 1.000000}, // quaternion (XYZW)
    .location = {0.000000, 0.000000, 0.000000},
  },
};

static const struct material materials[] = {
  {
    .name = "Untitled",
    .texture_id = 0,
  },
};
