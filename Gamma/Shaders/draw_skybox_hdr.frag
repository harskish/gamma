#version 330

out vec4 FragColor;
in vec3 dirWorld;
uniform samplerCube envMap;
  
void main()
{   
	// Tonemap and gamma-correct done as post process  
    FragColor = vec4(texture(envMap, dirWorld).rgb, 1.0);
}