@vs vs
in vec3 position;
in vec3 aColor;     // the color variable
  
out vec3 ourColor;  // output a color to the fragment shader 

void main() {
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
    ourColor = aColor;  // set ourColor to the input color we got from the vertex data
}
@end

@fs fs
in vec3 ourColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
@end

@program simple vs fs
