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
    gl_PointSize = gl_Position.z;    
}
@end

@fs fs
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.3, 0.6, 1.0);
}
@end

@program simple vs fs
