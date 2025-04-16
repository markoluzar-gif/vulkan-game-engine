#version 450

layout(location = 0) in vec3 vert_position;
layout(location = 1) in vec3 vert_color;
layout(location = 2) in vec2 vert_texture;
layout(location = 3) in ivec4 vert_bone_ids;
layout(location = 4) in vec4 vert_weights;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_texture;

layout(set = 0, binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 projection;
} view_projection;

layout(set = 0, binding = 1) uniform Bones {
    mat4 bone_matrices[512];
} bones;

layout(push_constant) uniform Model {
    mat4 model;
    int is_animated;
} model_pc;

void main() {
    vec4 pos = vec4(vert_position, 1.0);

    if (model_pc.is_animated == 1) {
        mat4 skin_matrix = mat4(0.0);
        skin_matrix += (vert_weights.x * bones.bone_matrices[vert_bone_ids.x]);
        skin_matrix += (vert_weights.y * bones.bone_matrices[vert_bone_ids.y]);
        skin_matrix += (vert_weights.z * bones.bone_matrices[vert_bone_ids.z]);
        skin_matrix += (vert_weights.w * bones.bone_matrices[vert_bone_ids.w]);
        pos = skin_matrix * pos;
    }

    gl_Position = view_projection.projection * view_projection.view * model_pc.model * pos;
    frag_color = vert_color;
    frag_texture = vert_texture;
}