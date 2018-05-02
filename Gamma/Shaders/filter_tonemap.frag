#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform bool useUC2;
uniform float exposure;

// UC2 parameters
float A = 0.22; // shoulder strength
float B = 0.30; // linear strength
float C = 0.10; // linear angle
float D = 0.20; // toe strength
float E = 0.01; // toe numerator
float F = 0.30; // tone denominator
float W = 11.2; // linear white point

vec3 Uncharted2Tonemap(vec3 x) {
     return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ReinhardTonemap(vec3 x) {
	return x / (1.0 + x);
}

void main() {
	vec3 color = texture(sourceTexture, TexCoords).rgb;
	color *= exposure; // exposure adjustment

	if (useUC2) {
		float exposureBias = 2.0;
		color = Uncharted2Tonemap(exposureBias * color) / Uncharted2Tonemap(vec3(W));
		color = pow(color, vec3(1.0/2.2));
	}
	else {
		color = ReinhardTonemap(color);
		color = pow(color, vec3(1.0/2.2));
	}

	FragColor = vec4(color, 1.0);
}