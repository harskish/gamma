#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform vec2 blurScale;

// Separable 7x1 Gaussian blur filter
void main() {
	vec4 color = vec4(0.0);

	color += texture(sourceTexture, TexCoords + (vec2(-3.0) * blurScale)) * (1.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(-2.0) * blurScale)) * (6.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(-1.0) * blurScale)) * (15.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(0.0) * blurScale)) * (20.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(1.0) * blurScale)) * (15.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(2.0) * blurScale)) * (6.0/64.0);
	color += texture(sourceTexture, TexCoords + (vec2(3.0) * blurScale)) * (1.0/64.0);


    FragColor = color;
}