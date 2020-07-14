@ctype mat4 hmm_mat4

@vs vs
in vec3 position;
in vec2 aTexCoord;
  
out vec2 TexCoord;

uniform vs_params {
    mat4 transform;
};

void main() {
    gl_Position = transform * vec4(position, 1.0);
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
@end

@program simple vs fs
