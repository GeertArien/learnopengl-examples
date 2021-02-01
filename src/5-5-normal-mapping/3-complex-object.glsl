@ctype vec3 hmm_vec3
@ctype vec4 hmm_vec4
@ctype mat4 hmm_mat4

@vs vs
#define NR_POINT_LIGHTS 4

in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;
in vec3 a_tangent;

out INTERFACE {
    vec2 tex_coords;
    vec3 tangent_dir_light_direction;
    vec3 tangent_point_light_pos[NR_POINT_LIGHTS];
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
} inter;

uniform vs_params {
    mat4 view;
    mat4 projection;
    mat4 model;
    vec3 view_pos;
};

uniform vs_dir_light {
    vec3 direction;
} dir_light;

// using arrays of vec4 to avoid alignment issues with cross shader compilatio
uniform vs_point_lights {
    vec4 position[NR_POINT_LIGHTS];
} point_lights;


mat3 transpose(mat3 mat) {
    vec3 i0 = mat[0];
    vec3 i1 = mat[1];
    vec3 i2 = mat[2];

    return mat3(
        vec3(i0.x, i1.x, i2.x),
        vec3(i0.y, i1.y, i2.y),
        vec3(i0.z, i1.z, i2.z)
    );
}

void main() {
    inter.tex_coords = a_tex_coords;
    
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    mat3 normal_matrix = mat3(model);;
    vec3 T = normalize(normal_matrix * a_tangent);
    vec3 N = normalize(normal_matrix * a_normal);
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));
    // TBN does not perform non-uniform scale, so we don't need inverse transpose for the direction
    inter.tangent_dir_light_direction = TBN * dir_light.direction;
    for(int i = 0; i < NR_POINT_LIGHTS; ++i) {
        inter.tangent_point_light_pos[i] = TBN * point_lights.position[i].xyz;
    }
    inter.tangent_view_pos  = TBN * view_pos;
    inter.tangent_frag_pos  = TBN * vec3(model * vec4(a_pos, 1.0));
        
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs
#define NR_POINT_LIGHTS 4

in INTERFACE {
    vec2 tex_coords;
    vec3 tangent_dir_light_direction;
    vec3 tangent_point_light_pos[NR_POINT_LIGHTS];
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
} inter;

out vec4 frag_color;

uniform fs_params {
    float material_shininess;
    float normal_mapping;
};

uniform fs_dir_light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dir_light;

// using arrays of vec4 to avoid alignment issues with cross shader compilation
uniform fs_point_lights {
    vec4 ambient[NR_POINT_LIGHTS];
    vec4 diffuse[NR_POINT_LIGHTS];
    vec4 specular[NR_POINT_LIGHTS];
    vec4 attenuation[NR_POINT_LIGHTS];
} point_lights;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;

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

dir_light_t get_directional_light() {
    return dir_light_t(
        inter.tangent_dir_light_direction,
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
                inter.tangent_point_light_pos[i],
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

vec3 calc_dir_light(dir_light_t light, vec3 normal, vec3 view_dir) {
    vec3 light_dir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);
    // specular shading
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, halfway_dir), 0.0), material_shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuse_map, inter.tex_coords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuse_map, inter.tex_coords));
    vec3 specular = light.specular * spec * vec3(texture(specular_map, inter.tex_coords));
    return (ambient + diffuse + specular);
}

vec3 calc_point_light(point_light_t light, vec3 normal, vec3 frag_pos, vec3 view_dir) {
    vec3 light_dir = normalize(light.position - frag_pos);
    // diffuse shading
    float diff = max(dot(normal, light_dir), 0.0);
    // specular shading
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, halfway_dir), 0.0), material_shininess);
    // attenuation
    float distance    = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuse_map, inter.tex_coords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuse_map, inter.tex_coords));
    vec3 specular = light.specular * spec * vec3(texture(specular_map, inter.tex_coords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main() {           
    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normal_map, inter.tex_coords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);
    // set normal to Z+ if normal mapping is disabled
    normal = normal_mapping == 1.0 ? normal : vec3(0.0, 0.0, 1.0);
    vec3 view_dir = normalize(inter.tangent_view_pos - inter.tangent_frag_pos);
    
    // phase 1: Directional lighting
    vec3 result = calc_dir_light(get_directional_light(), normal, view_dir);
    // phase 2: Point lights
    for(int i = 0; i < NR_POINT_LIGHTS; ++i) {
        result += calc_point_light(get_point_light(i), normal, inter.tangent_frag_pos, view_dir);
    }
    
    frag_color = vec4(result, 1.0);
}
@end

@program blinn_phong vs fs
