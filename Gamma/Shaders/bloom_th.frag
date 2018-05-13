#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform float threshold;

// ITU BT.601
float luminance(vec3 color) {
	return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

void main() {
	vec3 color = texture(sourceTexture, TexCoords).rgb;
	float luma = luminance(color);
	
	if (luma < threshold)
		color = vec3(0.0);
	
	FragColor = vec4(color, 1.0);
}