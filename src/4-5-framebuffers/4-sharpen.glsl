@ctype vec2 hmm_vec2
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

uniform fs_params {
    vec2 offset;
};

void main() {
    /* GLSL ES 1.0 (WebGL 1.0) does not support array constructor */
    vec2 offsets[9];
    offsets[0] = vec2(-offset.x,  offset.y); // top-left
    offsets[1] = vec2( 0.0,       offset.y); // top-center
    offsets[2] = vec2( offset.x,  offset.y); // top-right
    offsets[3] = vec2(-offset.x,  0.0);   // center-left
    offsets[4] = vec2( 0.0,       0.0);   // center-center
    offsets[5] = vec2( offset.x,  0.0);   // center-right
    offsets[6] = vec2(-offset.x, -offset.y); // bottom-left
    offsets[7] = vec2( 0.0,      -offset.y); // bottom-center
    offsets[8] = vec2( offset.x, -offset.y); // bottom-right    

    float kernel[9];
    kernel[0] = kernel[1] = kernel[2] = kernel[3] = -1.;
    kernel[4] = 9.;
    kernel[5] = kernel[6] = kernel[7] = kernel[8] = -1.;
    
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
