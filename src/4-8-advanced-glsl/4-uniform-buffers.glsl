@ctype mat4 hmm_mat4

@vs vs
in vec3 aPos;

layout(binding=0) uniform vs_view_projection {
    mat4 view;
    mat4 projection;
};

layout(binding=1) uniform vs_model {
    mat4 model;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
@end

@fs fs_red
out vec4 FragColor;

void main() {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
@end

@fs fs_green
out vec4 FragColor;

void main() {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
@end

@fs fs_blue
out vec4 FragColor;

void main() {
        FragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
@end

@fs fs_yellow
out vec4 FragColor;

void main() {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}
@end


@program red vs fs_red
@program green vs fs_green
@program blue vs fs_blue
@program yellow vs fs_yellow
