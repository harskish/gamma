#version 330

layout(location = 0) in vec3 posAttrib;
uniform mat4 lightSpaceMatrix;
uniform mat4 M;

void main() {
	gl_Position = lightSpaceMatrix * M * vec4(posAttrib, 1.0);
}