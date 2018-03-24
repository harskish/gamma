#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
  
void main() {
    FragColor = texture(sourceTexture, TexCoords);
}