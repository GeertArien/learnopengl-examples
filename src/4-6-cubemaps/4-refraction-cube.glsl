@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 a_pos;
in vec3 a_normal;

out vec3 normal;
out vec3 position;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    normal = mat3(model) * a_normal;
    position = vec3(model * vec4(a_pos, 1.0));
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs
in vec3 normal;
in vec3 position;

out vec4 frag_color;

uniform fs_params {
    vec3 camera_pos;
};

uniform samplerCube skybox_texture;

void main() {
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(position - camera_pos);
    vec3 R = refract(I, normalize(normal), ratio);
    frag_color = vec4(texture(skybox_texture, R).rgb, 1.0);
}
@end

@vs vs_skybox
in vec3 a_pos;

out vec3 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    tex_coords = a_pos;
    vec4 pos = projection * view * vec4(a_pos, 1.0);
    gl_Position = pos.xyww;
}
@end

@fs fs_skybox
in vec3 tex_coords;

out vec4 frag_color;

uniform samplerCube skybox_texture;

void main() {
    frag_color = texture(skybox_texture, tex_coords);
}
@end

@program simple vs fs
@program skybox vs_skybox fs_skybox
