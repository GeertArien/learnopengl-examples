@ctype mat4 hmm_mat4

@vs vs
in vec3 aPos;
in vec2 aTexCoord;
  
out vec2 TexCoord;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    // note that we read the multiplication from right to left
    gl_Position = projection * view * model * vec4(aPos, 1.0);
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
