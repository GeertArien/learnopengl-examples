@vs vs
in vec4 position;

void main() {
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
}
@end

@fs fs
uniform fs_params {
    vec4 ourColor;
};

out vec4 FragColor;

void main() {
    FragColor = ourColor;
}
@end

@program simple vs fs
