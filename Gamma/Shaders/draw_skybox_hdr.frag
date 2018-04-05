#version 330

out vec4 FragColor;
in vec3 dirWorld;
uniform samplerCube envMap;
  
void main()
{
    vec3 envColor = texture(envMap, dirWorld).rgb;
    
	// Tonemap, gamma-correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));
  
    FragColor = vec4(envColor, 1.0);
}