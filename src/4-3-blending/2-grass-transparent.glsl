@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 a_pos;
in vec2 a_tex_coords;

out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    tex_coords = a_tex_coords;
}
@end

@fs fs
in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@fs fs_grass
in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main() {
    vec4 tex_color = texture(diffuse_texture, tex_coords);
    if (tex_color.a < 0.1)
        discard;
    frag_color = tex_color;
}
@end

@program simple vs fs
@program grass vs fs_grass
