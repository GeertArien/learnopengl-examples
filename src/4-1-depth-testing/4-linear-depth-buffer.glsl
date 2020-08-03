@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
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

@fs fs
in vec2 tex_coords;

out vec4 frag_color;

float near = 0.1; 
float far  = 100.0; 
  
float linearize_depth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{             
    float depth = linearize_depth(gl_FragCoord.z) / far; // divide by far for demonstration
    frag_color = vec4(vec3(depth), 1.0);
}
@end

@program phong vs fs
