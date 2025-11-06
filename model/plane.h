#pragma once

const vec3 plane_position[] = {
    {1.000000f, 1.000000f, -0.000000f},
    {1.000000f, -1.000000f, -0.000000f},
    {-1.000000f, 1.000000f, 0.000000f},
    {-1.000000f, -1.000000f, 0.000000f},
};

const vec2 plane_texture[] = {
    {1.000000f, 0.000000f},
    {0.000000f, 1.000000f},
    {0.000000f, 0.000000f},
    {1.000000f, 1.000000f},
};

const vec3 plane_normal[] = {
    {-0.0000f, -0.0000f, -1.0000f},
};

const triangle_t plane_Plane_triangle[] = {
    {
        {1, 0, 0},
        {2, 1, 0},
        {0, 2, 0},
    },
    {
        {1, 0, 0},
        {3, 3, 0},
        {2, 1, 0},
    },
};

const object plane_Plane = {
    .triangle = &plane_Plane_triangle[0],
    .triangle_count = 2,
};

const object * plane_object[] = {
    &plane_Plane,
};

const model plane_model = {
    .position = plane_position,
    .texture = plane_texture,
    .normal = plane_normal,
    .object = plane_object,
    .object_count = 1
};

