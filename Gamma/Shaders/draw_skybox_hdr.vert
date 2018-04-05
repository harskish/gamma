#version 330
layout (location = 0) in vec3 posAttrib;

uniform mat4 P;
uniform mat4 V;
out vec3 dirWorld;

void main()
{
    dirWorld = posAttrib; // assume camera at origin

    mat4 rotView = mat4(mat3(V)); // remove translation
    vec4 clipPos = P * rotView * vec4(posAttrib, 1.0);

    gl_Position = clipPos.xyww;
}