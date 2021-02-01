@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;

out vec3 frag_pos;
out vec3 normal;
out vec2 tex_coords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
    frag_pos = vec3(model * vec4(a_pos, 1.0));
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    normal = mat3(model) * a_normal;
    tex_coords = a_tex_coords;
}
@end

@fs fs
in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coords;

out vec4 frag_color;

uniform fs_params {
    vec3 view_pos;
    float material_shininess;
};

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;

uniform fs_dir_light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dir_light;

#define NR_POINT_LIGHTS 4  

// using arrays of vec4 to avoid alignment issues with cross shader compilation
uniform fs_point_lights {
    vec4 position[NR_POINT_LIGHTS];  
    vec4 ambient[NR_POINT_LIGHTS];
    vec4 diffuse[NR_POINT_LIGHTS];
    vec4 specular[NR_POINT_LIGHTS];
    vec4 attenuation[NR_POINT_LIGHTS];
} point_lights;

uniform fs_spot_light {
    vec3 position;
    vec3 direction;
    float cut_off;
    float outer_cut_off;
    vec3 attenuation;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} spot_light;

// directional light type
struct dir_light_t {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

struct point_light_t {    
    vec3 position;
    float constant;
    float linear;
    float quadratic;  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct spot_light_t {
    vec3 position;
    vec3 direction;
    float cut_off;
    float outer_cut_off;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

dir_light_t get_directional_light();
point_light_t get_point_light(int index);
spot_light_t get_spot_light();

vec3 calc_dir_light(dir_light_t light, vec3 normal, vec3 view_dir);
vec3 calc_point_light(point_light_t light, vec3 normal, vec3 frag_pos, vec3 view_dir);
vec3 calc_spot_light(spot_light_t light, vec3 normal, vec3 frag_pos, vec3 view_dir);

void main() {
    // properties
    vec3 norm = normalize(normal);
    vec3 view_dir = normalize(view_pos - frag_pos);

    // phase 1: Directional lighting
    vec3 result = calc_dir_light(get_directional_light(), norm, view_dir);
    // phase 2: Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; ++i) {
        result += calc_point_light(get_point_light(i), norm, frag_pos, view_dir);
    }
    // phase 3: Spot light
    result += calc_spot_light(get_spot_light(), norm, frag_pos, view_dir);
    
    frag_color = vec4(result, 1.0);
}

dir_light_t get_directional_light() {
    return dir_light_t(
        dir_light.direction,
        dir_light.ambient,
        dir_light.diffuse,
        dir_light.specular
    );
}

point_light_t get_point_light(int index) {
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        // workaround because gles2 only allows dynamic array indices
        // with constant expressions or loop indices
        if (i == index) {
            return point_light_t(
                point_lights.position[i].xyz,
                point_lights.attenuation[i].x,
                point_lights.attenuation[i].y,
                point_lights.attenuation[i].z,
                point_lights.ambient[i].xyz,
                point_lights.diffuse[i].xyz,
                point_lights.specular[i].xyz
            );
        }
    }
}

spot_light_t get_spot_light() {
    return spot_light_t(
        spot_light.position,
        spot_light.direction,
        spot_light.cut_off,
        spot_light.outer_cut_off,
        spot_light.attenuation.x,
        spot_light.attenuation.y,
        spot_light.attenuation.z,
        spot_light.ambient,
        spot_light.diffuse,
        spot_light.specular
    );
}

vec3 calc_dir_light(dir_light_t light, vec3 normal, vec3 view_dir) {
    vec3 light_dir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);
    // specular shading
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material_shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuse_texture, tex_coords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuse_texture, tex_coords));
    vec3 specular = light.specular * spec * vec3(texture(specular_texture, tex_coords));
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(point_light_t light, vec3 normal, vec3 frag_pos, vec3 view_dir) {
    vec3 light_dir = normalize(light.position - frag_pos);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);
    // specular shading
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material_shininess);
    // attenuation
    float distance    = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuse_texture, tex_coords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuse_texture, tex_coords));
    vec3 specular = light.specular * spec * vec3(texture(specular_texture, tex_coords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 calc_spot_light(spot_light_t light, vec3 normal, vec3 frag_pos, vec3 view_dir) {
    vec3 light_dir = normalize(light.position - frag_pos);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);
    // specular shading
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material_shininess);
    // attenuation
    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(light_dir, normalize(-light.direction)); 
    float epsilon = light.cut_off - light.outer_cut_off;
    float intensity = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse_texture, tex_coords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_texture, tex_coords));
    vec3 specular = light.specular * spec * vec3(texture(specular_texture, tex_coords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
@end

@vs light_cube_vs
in vec3 a_pos;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs light_cube_fs
out vec4 frag_color;

void main() {
    frag_color = vec4(1.0);      // set all 4 vector values to 1.0
}
@end

@program phong vs fs
@program light_cube light_cube_vs light_cube_fs
