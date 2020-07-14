@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 aPos;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
@end

@fs fs
out vec4 FragColor;

uniform fs_params {
    vec3 objectColor;
    vec3 lightColor;
};

void main() {
    FragColor = vec4(lightColor * objectColor, 1.0);
}
@end

@fs light_cube_fs
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0);      // set all 4 vector values to 1.0
}
@end


@program simple vs fs
@program light_cube vs light_cube_fs
