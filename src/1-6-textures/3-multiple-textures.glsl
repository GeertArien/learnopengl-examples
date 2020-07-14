@vs vs
in vec3 position;
in vec3 aColor;
in vec2 aTexCoord;
  
out vec3 ourColor;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}
@end

@program simple vs fs
