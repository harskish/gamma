#version 330 core
layout (location = 0) in vec3 posAttrib;

uniform mat4 M;

void main() {
	gl_Position = M * vec4(posAttrib, 1.0);
}