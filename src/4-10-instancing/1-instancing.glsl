@ctype vec4 hmm_vec4

@vs vs
in vec2 aPos;
in vec3 aColor; 
out vec3 fColor;

uniform vs_params {
    // using arrays vec4 to avoid alignment issues with cross shader compilation
    vec4 offsets[100];
};

void main() {
    vec2 offset = offsets[gl_InstanceID].xy;
    gl_Position = vec4(aPos + offset, 0.0, 1.0);
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
