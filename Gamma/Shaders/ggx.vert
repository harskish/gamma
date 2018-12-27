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
uniform mat4 M_it; // inverse transpose

void main() {
	TexCoords = texAttrib;
	WorldPos = vec3(M * vec4(posAttrib, 1.0));
	Normal = vec3(M_it * vec4(normAttrib, 0.0));
					
	gl_Position = P * V * vec4(WorldPos, 1.0);
}
