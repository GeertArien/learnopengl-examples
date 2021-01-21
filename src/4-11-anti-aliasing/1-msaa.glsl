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

void main() {
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs fs
