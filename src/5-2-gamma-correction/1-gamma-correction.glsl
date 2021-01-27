@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;

// declare an interface block; see 'Advanced GLSL' for what these are.
out INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} inter;

uniform vs_params {
    mat4 view;
    mat4 projection;
};

void main() {
    inter.frag_pos = a_pos;
    inter.normal = a_normal;
    inter.tex_coords = a_tex_coords;
    gl_Position = projection * view * vec4(a_pos, 1.0);
}
@end

@fs fs
in INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} inter;

out vec4 frag_color;

// using arrays of vec4 to avoid alignment issues with cross shader compilation
uniform fs_params {
    vec4 light_pos[4];
    vec4 light_colors[4];
    vec3 view_pos;
    float gamma;        // the shader cross compiler does not support bool as uniform
};

uniform sampler2D floor_texture;

vec3 blinnPhong(vec3 normal, vec3 frag_pos, vec3 light_pos, vec3 light_color) {
    // diffuse
    vec3 light_dir = normalize(light_pos - frag_pos);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * light_color;
    // specular
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = 0.0;
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    spec = pow(max(dot(normal, halfway_dir), 0.0), 64.0);
    vec3 specular = spec * light_color;    
    // simple attenuation
    float max_distance = 1.5;
    float distance = length(light_pos - frag_pos);
    float attenuation = 1.0 / (gamma == 1.0 ? distance * distance : distance);
    
    diffuse *= attenuation;
    specular *= attenuation;
    
    return diffuse + specular;
}

void main() {           
    vec3 color = texture(floor_texture, inter.tex_coords).rgb;
    // if gamma correction is enabled we need to transform texture colors to linear space first
    // sokol does not support srgb as pixelformat
    if(gamma == 1.0)
        color = pow(color, vec3(2.2));
    
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < 4; ++i)
        lighting += blinnPhong(normalize(inter.normal), inter.frag_pos, light_pos[i].xyz, light_colors[i].rgb);
    color *= lighting;
    
    if(gamma == 1.0)
        color = pow(color, vec3(1.0/2.2));
    frag_color = vec4(color, 1.0);
}
@end

@program blinn_phong vs fs
