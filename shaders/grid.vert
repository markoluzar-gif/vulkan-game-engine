#version 450

layout(set = 0, binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 projection;
} view_projection;

layout(location = 2) out vec3 near_point;
layout(location = 3) out vec3 far_point;

layout(push_constant) uniform Model {
    mat4 model;
} model;

vec3 grid_plane[6] = vec3[](
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unproject_point(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 view_inv = inverse(view);
    mat4 proj_inv = inverse(projection);
    vec4 unprojected_point = view_inv * proj_inv * vec4(x, y, z, 1.0);
    return unprojected_point.xyz / unprojected_point.w;
}

void main() {
    vec3 p = grid_plane[gl_VertexIndex];
    near_point = unproject_point(p.x, p.y, 0.0, view_projection.view, view_projection.projection);
    far_point  = unproject_point(p.x, p.y, 1.0, view_projection.view, view_projection.projection);
    gl_Position = vec4(p, 1.0);
}