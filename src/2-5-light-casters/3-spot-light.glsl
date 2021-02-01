@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    // inverse tranpose is left out because:
    // (a) glsl es 1.0 (webgl 1.0) doesn't have inverse and transpose functions
    // (b) we're not performing non-uniform scale
    Normal = mat3(model) * aNormal;
    TexCoords = aTexCoords;
}
@end

@fs fs
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform fs_params {
    vec3 viewPos;
};

uniform fs_material {
    float shininess;
} material;

uniform fs_light {
    vec3 position;  
    vec3 direction;
    float cut_off;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
} light;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;

void main() {
    // attenuation
    float distance    = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance)); 

    // ambient
    vec3 ambient = light.ambient * vec3(texture(diffuse_texture, TexCoords)); 
    ambient *= attenuation;

    vec3 lightDir = normalize(light.position - FragPos);

    // check if lighting is inside the spotlight cone
    float theta = dot(lightDir, normalize(-light.direction)); 
    
    if(theta > light.cut_off) // remember that we're working with angles as cosines instead of degrees so a '>' is used.
    {   
        // diffuse 
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_texture, TexCoords));
        
        // specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * spec * vec3(texture(specular_texture, TexCoords));

        diffuse *= attenuation;
        specular *= attenuation;   
        
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    }
    else
    {
        // else, use ambient light so scene isn't completely dark outside the spotlight.
        FragColor = vec4(ambient, 1.0);
    }
}
@end

@program phong vs fs
