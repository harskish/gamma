#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D tex1;
uniform sampler2D tex2;
  
void main() {
	FragColor = texture(tex1, TexCoords) + texture(tex2, TexCoords);
}