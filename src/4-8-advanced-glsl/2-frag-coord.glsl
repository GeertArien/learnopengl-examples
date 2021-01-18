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
uniform fs_params {
    float center_x;
};

out vec4 FragColor;

void main() {
    if(gl_FragCoord.x < center_x)
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
@end

@program simple vs fs
