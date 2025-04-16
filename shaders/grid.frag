#version 450

layout(location = 2) in vec3 near_point;
layout(location = 3) in vec3 far_point;

layout(set = 0, binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 projection;
} view_projection;

layout(set = 0, binding = 1) uniform Movement {
    float camera_speed;
} movement;

layout(location = 0) out vec4 out_color;

vec4 grid(vec3 frag_pos_3d, float scale, bool draw_axis) {
    vec2 coord = frag_pos_3d.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid_dist = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = 1.0 - smoothstep(0.0, 1.0, min(grid_dist.x, grid_dist.y));
    vec4 color = vec4(0.2, 0.2, 0.2, line);

    if (draw_axis) {
        if (abs(frag_pos_3d.x) < derivative.x * 0.1)
            color.z = 1.0;
        if (abs(frag_pos_3d.z) < derivative.y * 0.1)
            color.x = 1.0;
    }
    return color;
}

float compute_depth(vec3 pos) {
    vec4 clip_space_pos = view_projection.projection * view_projection.view * vec4(pos, 1.0);
    return clip_space_pos.z / clip_space_pos.w;
}

float compute_linear_depth(vec3 pos) {
    vec4 clip_space_pos = view_projection.projection * view_projection.view * vec4(pos, 1.0);
    float clip_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0;
    float linear_depth = (2.0 * 0.01 * 100.0) / (100.0 + 0.01 - clip_depth * (100.0 - 0.01));
    return linear_depth / 100.0;
}

void main() {
    float t = -near_point.y / (far_point.y - near_point.y);
    vec3 frag_pos_3d = near_point + t * (far_point - near_point);
    gl_FragDepth = compute_depth(frag_pos_3d);

    float linear_depth = compute_linear_depth(frag_pos_3d);
    float depth_fade = smoothstep(0.3, 0.6, linear_depth);
    float fine_grid_factor = (1.0 - depth_fade) * (1.0 - movement.camera_speed);

    vec4 coarse_grid = grid(frag_pos_3d, 10.0, true);
    vec4 fine_grid = grid(frag_pos_3d, 1.0, true) * fine_grid_factor;

    vec4 color = coarse_grid + fine_grid;
    color.a *= (1.0 - depth_fade);

    out_color = color;
}