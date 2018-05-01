#version 330

out vec4 FragColor;
in vec4 FragPos; // world pos point on cube face
in mat4 View;

uniform samplerCube sourceTexture;
uniform vec3 blurScale;

// Separable 7x1 Gaussian blur filter applied onto cubemap face
// Make sure GL_TEXTURE_CUBE_MAP_SEAMLESS is turned on for seamless edges

void main() {
	vec3 dir = FragPos.xyz;
	
	// Screen space to world space offset
	mat3 T = transpose(mat3(View)); // R^-1 = R^T

	vec4 color = vec4(0.0);
	color += texture(sourceTexture, dir + T * (vec3(-3.0) * blurScale)) * (1.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(-2.0) * blurScale)) * (6.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(-1.0) * blurScale)) * (15.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(0.0) * blurScale)) * (20.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(1.0) * blurScale)) * (15.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(2.0) * blurScale)) * (6.0/64.0);
	color += texture(sourceTexture, dir + T * (vec3(3.0) * blurScale)) * (1.0/64.0);

    FragColor = color;
}