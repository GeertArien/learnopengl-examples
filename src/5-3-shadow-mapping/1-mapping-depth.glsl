//------------------------------------------------------------------------------
//  float/rgba8 encoding/decoding so that we can use an RGBA8
//  shadow map instead of floating point render targets which might
//  not be supported everywhere
//
//  http://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/
//

@ctype mat4 hmm_mat4

@vs vs_depth
in vec3 a_pos;

uniform vs_params {
    mat4 light_space_matrix;
    mat4 model;
};

void main() {
    gl_Position = light_space_matrix * model * vec4(a_pos, 1.0);
}
@end

@fs fs_depth
out vec4 frag_color;

vec4 encodeDepth(float v) {
    vec4 enc = vec4(1.0, 255.0, 65025.0, 16581375.0) * v;
    enc = fract(enc);
    enc -= enc.yzww * vec4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);
    return enc;
}

void main() {             
    // sokol and webgl 1 do not support using the depth map as texture
    // so instead we write the depth value to the color map
    frag_color = encodeDepth(gl_FragCoord.z);
}
@end

@vs vs_quad 
in vec3 a_pos;
in vec2 a_tex_coords;
out vec2 tex_coords;

void main() {
    tex_coords = a_tex_coords;
    gl_Position = vec4(a_pos, 1.0);
}
@end

@fs fs_quad
out vec4 frag_color;
in vec2 tex_coords;

uniform sampler2D depth_map;

float decodeDepth(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

void main() {             
    float depth_value = decodeDepth(texture(depth_map, tex_coords));
    frag_color = vec4(vec3(depth_value), 1.0); 
}
@end

@program depth vs_depth fs_depth
@program quad vs_quad fs_quad
