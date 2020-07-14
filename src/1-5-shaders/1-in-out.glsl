@vs vs
in vec4 position;

out vec4 vertexColor; // specify a color output to the fragment shader

void main() {
    gl_Position = vec4(position.x, position.y, position.z, 1.0);
    vertexColor = vec4(0.5, 0.0, 0.0, 1.0); // set the output variable to a dark-red color
}
@end

@fs fs
in vec4 vertexColor; // the input variable from the vertex shader (same name and same type)  

out vec4 FragColor;

void main() {
    FragColor = vertexColor;
}
@end

@program simple vs fs
