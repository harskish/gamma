#version 330
layout (location = 0) in vec3 posAttrib;

out vec3 posWorld;

uniform mat4 P;
uniform mat4 V;

void main() {
    posWorld = posAttrib;
    gl_Position =  P * V * vec4(posWorld, 1.0);
}