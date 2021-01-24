@ctype mat4 hmm_mat4

@block data
float getVal(uint index, sampler2D texture) {
    float x = index % 1024;
    float y = index / 1024;
    return texelFetch(texture, ivec2(x, y), 0).r;
}

vec2 getVec2(uint index, sampler2D texture) {
    float x = getVal(index, texture);
    float y = getVal(index + 1, texture);
    return vec2(x, y);
}

vec3 getVec3(uint index, sampler2D texture) {
    float x = getVal(index, texture);
    float y = getVal(index + 1, texture);
    float z = getVal(index + 2, texture);
    return vec3(x, y, z);
}
@end

@vs vs_simple
@include_block data
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains
out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

uniform sampler2D vertex_texture;

void main() {
    uint index = gl_VertexID * 5;
    vec4 pos = vec4(getVec3(index, vertex_texture), 1.0);
    tex_coords = getVec2(index + 3, vertex_texture);
    gl_Position = projection * view * model * pos;
    
}
@end

@fs fs_simple
in vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@vs vs_normals
@include_block data
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

uniform sampler2D vertex_texture;

const float MAGNITUDE = 0.2;

vec3 getNormal(vec3 p0, vec3 p1, vec3 p2) {
    vec3 a = p0 - p1;
    vec3 b = p1 - p2;
    return normalize(cross(a, b));
}

void main() {
    uint index = gl_VertexID >> 1;      // divide by 2
    index = index * 15;

    vec3 p0 = getVec3(index, vertex_texture);
    vec3 p1 = getVec3(index + 5, vertex_texture);
    vec3 p2 = getVec3(index + 10, vertex_texture);

    vec3 mid = (p0 + p1 + p2) / 3.0;
    vec3 normal = getNormal(p0, p1, p2);
    vec3 direction = (gl_VertexID % 2) * normal; 

    vec4 pos = vec4(mid + direction * MAGNITUDE, 1.0);
    gl_Position = projection * view * model * pos;
}
@end

@fs fs_normals
out vec4 frag_color;

void main() {
    frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs_simple fs_simple
@program normals vs_normals fs_normals
