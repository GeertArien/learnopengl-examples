@ctype vec4 hmm_vec4

@vs vs
in float a_dummy;       // add a dummy vertex attribute otherwise sokol complains

uniform sampler2D positions_texture;

void main() {
    uint index = gl_VertexID >> 1;      // divide by 2
    vec4 pos = texelFetch(positions_texture, ivec2(index, 0), 0);

    pos.x = gl_VertexID % 2 == 0 ? pos.x - 0.1 : pos.x + 0.1;
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}
@end

@fs fs
out vec4 frag_color;

void main() {
    frag_color = vec4(0.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs fs
