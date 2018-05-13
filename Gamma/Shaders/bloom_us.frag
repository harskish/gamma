#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform vec2 invRes;

// Parameters
uniform float radius;
uniform float strength;

void main() {
	vec2 offset = invRes * radius;

	vec4 c0 = texture(sourceTexture, TexCoords + vec2(-1, -1) * offset);
	vec4 c1 = texture(sourceTexture, TexCoords + vec2(0, -1) * offset);
	vec4 c2 = texture(sourceTexture, TexCoords + vec2(1, -1) * offset);
	vec4 c3 = texture(sourceTexture, TexCoords + vec2(-1, 0) * offset);
	vec4 c4 = texture(sourceTexture, TexCoords);
	vec4 c5 = texture(sourceTexture, TexCoords + vec2(1, 0) * offset);
	vec4 c6 = texture(sourceTexture, TexCoords + vec2(-1,1) * offset);
	vec4 c7 = texture(sourceTexture, TexCoords + vec2(0, 1) * offset);
	vec4 c8 = texture(sourceTexture, TexCoords + vec2(1, 1) * offset);

    // Tentfilter
	FragColor =  0.0625 * (c0 + 2.0 * c1 + c2 + 2.0 * c3 + 4.0 * c4 + 2.0 * c5 + c6 + 2.0 * c7 + c8) * strength;
}