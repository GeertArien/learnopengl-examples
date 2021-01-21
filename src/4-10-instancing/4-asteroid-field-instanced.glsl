@ctype mat4 hmm_mat4

@vs vs_planet
in vec3 a_pos;
in vec2 a_tex_coords;
out vec2 tex_coords;

uniform vs_params_planet {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    tex_coords = a_tex_coords;
}
@end

@vs vs_rock
in vec3 a_pos;
in vec2 a_tex_coords;
in vec4 instance_mat0;
in vec4 instance_mat1;
in vec4 instance_mat2;
in vec4 instance_mat3;
out vec2 tex_coords;

uniform vs_params_rock {
    mat4 view;
    mat4 projection;
};

void main() {
    mat4 instance_matrix = mat4(instance_mat0, instance_mat1, instance_mat2, instance_mat3);
    gl_Position = projection * view * instance_matrix * vec4(a_pos, 1.0);
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

@program planet vs_planet fs
@program rock vs_rock fs
