@ctype mat4 hmm_mat4

@vs vs_offscreen
in vec3 a_pos;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs_offscreen
out vec4 FragColor;

void main() {
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
@end


@vs vs_display
in vec2 a_pos;
in vec2 a_tex_coords;
out vec2 tex_coords;

void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
    tex_coords = a_tex_coords;
}
@end

@fs fs_display
in vec2 tex_coords;
out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main() {
    vec4 color = texture(diffuse_texture, tex_coords);
    float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    frag_color = vec4(average, average, average, 1.0);
}
@end

@program offscreen vs_offscreen fs_offscreen
@program display vs_display fs_display
