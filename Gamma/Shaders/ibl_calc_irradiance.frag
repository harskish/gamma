#version 330

out vec4 FragColor;
in vec3 posWorld;
uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {
    vec3 normal = normalize(posWorld); // hemisphere orientation
    vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = 0.025;
	float nrSamples = 0.0; 
	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			vec3 dirLocal = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 dirWorld = dirLocal.x * right + dirLocal.y * up + dirLocal.z * normal;

			irradiance += texture(environmentMap, dirWorld).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	
	irradiance = PI * irradiance * (1.0 / float(nrSamples));
    FragColor = vec4(irradiance, 1.0);
}