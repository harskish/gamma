#version 330

layout (location = 0) in vec4 posAttrib;

out vec3 velocity;
out vec3 position;
out vec3 posEye;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform float pointRadius;

void main() {
    velocity = vec3(0.0, 0.0, 0.0);
    position = posAttrib.xyz;
    posEye = vec3(V * M * vec4(position, 1.0));

    float dist = length(posEye);
    gl_PointSize = pointRadius / dist;

    gl_Position = P * V * M * vec4(position, 1.0);
}



