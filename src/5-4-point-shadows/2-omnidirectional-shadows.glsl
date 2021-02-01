//------------------------------------------------------------------------------
//  float/rgba8 encoding/decoding so that we can use an RGBA8
//  shadow map instead of floating point render targets which might
//  not be supported everywhere
//
//  http://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/
//

@ctype vec2 hmm_vec2
@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@block vs_params
uniform vs_params {
    mat4 model;
};
@end

@vs vs_depth
@include_block vs_params
in vec3 a_pos;
out vec4 frag_pos;

uniform vs_params_depth {
    mat4 light_space_matrix;
};

void main() {
    frag_pos = model * vec4(a_pos, 1.0);
    gl_Position = light_space_matrix * frag_pos;
}
@end

@fs fs_depth
in vec4 frag_pos;
out vec4 frag_color;

uniform fs_params_depth {
    vec3 light_pos;
    float far_plane;
};

vec4 encodeDepth(float v) {
    vec4 enc = vec4(1.0, 255.0, 65025.0, 16581375.0) * v;
    enc = fract(enc);
    enc -= enc.yzww * vec4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);
    return enc;
}

void main() {             
    float light_distance = length(frag_pos.xyz - light_pos);
    
    // map to [0;1] range by dividing by far_plane
    light_distance = light_distance / far_plane;
    
    // write this as modified depth
    // sokol and webgl 1 do not support using the depth map as texture
    // so instead we write the depth value to the color map
    frag_color = encodeDepth(light_distance);
}
@end

@vs vs_shadows
@include_block vs_params
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;

out INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} inter;

uniform vs_params_shadows {
    mat4 projection;
    mat4 view;
    float normal_multiplier;
};

void main() {
    inter.frag_pos = vec3(model * vec4(a_pos, 1.0));
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    inter.normal = mat3(model) * a_normal;
    // a slight hack to make sure the outer large cube displays lighting from the 'inside' instead of the default 'outside'.
    inter.normal = normal_multiplier * inter.normal;
    inter.tex_coords = a_tex_coords;
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs_shadows
in INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} inter;

out vec4 frag_color;

uniform sampler2D diffuse_texture;
uniform samplerCube depth_map;

uniform fs_params_shadows {
    vec3 light_pos;
    vec3 view_pos;
    float far_plane;
};

float decodeDepth(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

float shadowCalculation(vec3 frag_pos) {
    // get vector between fragment position and light position
    vec3 frag_to_light = frag_pos - light_pos;
    // use the fragment to light vector to sample from the depth map    
    float closest_depth = decodeDepth(texture(depth_map, frag_to_light));
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closest_depth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float current_depth = length(frag_to_light);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = current_depth -  bias > closest_depth ? 1.0 : 0.0;        
    // display closest_depth as debug (to visualize depth cubemap)
    //frag_color = vec4(vec3(closest_depth / far_plane), 1.0);    
        
    return shadow;
}

void main() {           
    vec3 color = texture(diffuse_texture, inter.tex_coords).rgb;
    vec3 normal = normalize(inter.normal);
    vec3 light_color = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * color;
    // diffuse
    vec3 light_dir = normalize(light_pos - inter.frag_pos);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * light_color;
    // specular
    vec3 view_dir = normalize(view_pos - inter.frag_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = 0.0;
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    spec = pow(max(dot(normal, halfway_dir), 0.0), 64.0);
    vec3 specular = spec * light_color;    
    // calculate shadow
    float shadow = shadowCalculation(inter.frag_pos);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    frag_color = vec4(lighting, 1.0);
}
@end

@program depth vs_depth fs_depth
@program shadows vs_shadows fs_shadows
