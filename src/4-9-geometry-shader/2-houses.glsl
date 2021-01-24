@ctype vec4 hmm_vec4

@vs vs
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains
out vec4 color;

uniform sampler2D position_texture;
uniform sampler2D color_texture;

void main() {
    uint pos_index = gl_VertexID / 9;
    vec4 pos = texelFetch(position_texture, ivec2(pos_index, 0), 0);

    uint index = gl_VertexID % 9;
    vec2 offset = index == 0 ? vec2(-0.2, 0.2) : vec2(0.0, 0.0);
    offset = index == 1 ? vec2(0.2, -0.2) : offset;
    offset = index == 2 ? vec2(-0.2, -0.2) : offset;
    offset = index == 3 ? vec2(-0.2, 0.2) : offset;
    offset = index == 4 ? vec2(0.2, 0.2) : offset;
    offset = index == 5 ? vec2(0.2, -0.2) : offset;
    offset = index == 6 ? vec2(-0.2, 0.2) : offset;
    offset = index == 7 ? vec2(0.0, 0.4) : offset;
    offset = index == 8 ? vec2(0.2, 0.2) : offset;
    gl_Position = vec4(pos.x + offset.x, pos.y + offset.y, 0.0, 1.0);

    color = texelFetch(color_texture, ivec2(pos_index, 0), 0);
    color = index == 7 ? vec4(1.0, 1.0, 1.0, 1.0) : color;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program simple vs fs
