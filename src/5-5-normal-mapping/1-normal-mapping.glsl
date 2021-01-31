@ctype vec3 hmm_vec3
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
    gl_Position = projection * view * vec4(a_pos, 1.0);
    inter.frag_pos = a_pos;
    inter.normal = a_normal;
    inter.tex_coords = a_tex_coords;
}
@end

@fs fs
in INTERFACE {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coords;
} inter;

out vec4 frag_color;

uniform fs_params {
    vec3 view_pos;
    vec3 light_pos;
    float normal_mapping;        // the shader cross compiler does not support bool as uniform
};

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;

void main() {           
    vec3 color = texture(diffuse_map, inter.tex_coords).rgb;
    // ambient
    vec3 ambient = 0.05 * color;
    // diffuse
    vec3 light_dir = normalize(light_pos - inter.frag_pos);
    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normal_map, inter.tex_coords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);
    normal = normal_mapping == 1.0 ? normal : normalize(inter.normal);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 view_dir = normalize(view_pos - inter.frag_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec; // assuming bright white light color
    frag_color = vec4(ambient + diffuse + specular, 1.0);
}
@end

@program blinn_phong vs fs
