#version 330

layout(location = 0) in vec3 posAttrib;
layout(location = 1) in vec3 normAttrib;
layout(location = 2) in vec2 texAttrib;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
                
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main() {
	TexCoords = texAttrib;
	WorldPos = vec3(M * vec4(posAttrib, 1.0));
	Normal = mat3(M) * normAttrib;
					
	gl_Position = P * V * vec4(WorldPos, 1.0);
}