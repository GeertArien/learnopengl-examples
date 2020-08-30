@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs_offscreen
in vec3 a_pos;
in vec2 a_tex_coords;

out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    tex_coords = a_tex_coords;
}
@end

@vs vs_display
in vec2 a_pos;
in vec2 a_tex_coords;

out vec2 tex_coords;

void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
    tex_coords = a_tex_coords;
}
@end

@fs fs_offscreen
in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main() {
    frag_color = texture(diffuse_texture, tex_coords);
}
@end

@fs fs_display
in vec2 tex_coords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

const float offset = 1.0 / 300.0;  

void main() {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        1,  1, 1,
        1, -8, 1,
        1,  1, 1
    );
    
    vec3 sample_tex[9];
    for(int i = 0; i < 9; i++) {
        sample_tex[i] = vec3(texture(diffuse_texture, tex_coords.st + offsets[i]));
    }

    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; i++)
        color += sample_tex[i] * kernel[i];
    
    frag_color = vec4(color, 1.0);
}  
@end

@program offscreen vs_offscreen fs_offscreen
@program display vs_display fs_display
