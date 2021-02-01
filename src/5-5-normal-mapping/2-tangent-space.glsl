@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_tex_coords;
in vec3 a_tangent;

out INTERFACE {
    vec2 tex_coords;
    vec3 tangent_light_pos;
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
} inter;

uniform vs_params {
    mat4 view;
    mat4 projection;
    mat4 model;
    vec3 light_pos;
    vec3 view_pos;
};

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
    mat3 normal_matrix = mat3(model);
    vec3 T = normalize(normal_matrix * a_tangent);
    vec3 N = normalize(normal_matrix * a_normal);
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));    
    inter.tangent_light_pos = TBN * light_pos;
    inter.tangent_view_pos  = TBN * view_pos;
    inter.tangent_frag_pos  = TBN * vec3(model * vec4(a_pos, 1.0));
        
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
@end

@fs fs
in INTERFACE {
    vec2 tex_coords;
    vec3 tangent_light_pos;
    vec3 tangent_view_pos;
    vec3 tangent_frag_pos;
} inter;

out vec4 frag_color;

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;

void main() {           
    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normal_map, inter.tex_coords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
   
    // get diffuse color
    vec3 color = texture(diffuse_map, inter.tex_coords).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 light_dir = normalize(inter.tangent_light_pos - inter.tangent_frag_pos);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 view_dir = normalize(inter.tangent_view_pos - inter.tangent_frag_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    frag_color = vec4(ambient + diffuse + specular, 1.0);
}
@end

@program blinn_phong vs fs
