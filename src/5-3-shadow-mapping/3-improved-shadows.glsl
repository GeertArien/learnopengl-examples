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
    mat4 light_space_matrix;
    mat4 model;
};
@end

@vs vs_depth
@include_block vs_params
in vec3 a_pos;

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

@vs vs_shadows
@include_block vs_params
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;

out INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
    vec4 frag_pos_light_space;
} inter;

uniform vs_params_shadows {
    mat4 projection;
    mat4 view;
};

void main() {
    inter.frag_pos = vec3(model * vec4(a_pos, 1.0));
    inter.normal = transpose(inverse(mat3(model))) * a_normal;
    inter.tex_coords = a_tex_coords;
    inter.frag_pos_light_space = light_space_matrix * vec4(inter.frag_pos, 1.0);
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs_shadows
in INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
    vec4 frag_pos_light_space;
} inter;

out vec4 frag_color;

uniform sampler2D diffuse_texture;
uniform sampler2D shadow_map;

uniform fs_params_shadows {
    vec3 light_pos;
    vec3 view_pos;
    vec2 shadow_map_size;
};

float decodeDepth(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

float shadowCalculation(vec4 frag_pos_light_space) {
    // perform perspective divide
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    // transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;
    // get depth of current fragment from light's perspective
    float current_depth = proj_coords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(inter.normal);
    vec3 light_dir = normalize(light_pos - inter.frag_pos);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    // check whether current frag pos is in shadow
    // PCF
    float shadow = 0.0;
    vec2 texel_size = 1.0 / shadow_map_size;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcf_depth = decodeDepth(texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size)); 
            shadow += current_depth - bias > pcf_depth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(proj_coords.z > 1.0)
        shadow = 0.0;

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
    float shadow = shadowCalculation(inter.frag_pos_light_space);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    frag_color = vec4(lighting, 1.0);
}
@end

@program depth vs_depth fs_depth
@program shadows vs_shadows fs_shadows
