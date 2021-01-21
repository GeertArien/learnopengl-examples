@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 aPos;
in vec2 aTexCoords;

out vec2 TexCoords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}
@end

@fs fs
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D front_texture;
uniform sampler2D back_texture;

void main() {
    if(gl_FrontFacing)
        FragColor = texture(front_texture, TexCoords);
    else
        FragColor = texture(back_texture, TexCoords);
}
@end

@program simple vs fs
