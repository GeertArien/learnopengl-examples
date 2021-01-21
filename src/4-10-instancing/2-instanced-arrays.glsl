@vs vs
in vec2 aPos;
in vec3 aColor;
in vec2 aOffset;
out vec3 fColor;

void main() {
    vec2 pos = aPos * (gl_InstanceID / 100.0);
    gl_Position = vec4(pos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
@end

@fs fs
in vec3 fColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(fColor, 1.0);
}
@end

@program simple vs fs
