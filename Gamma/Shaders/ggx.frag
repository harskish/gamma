#version 330

#include "common.glh"
#include "ggx_funcs.glh"
#include "shadow_funcs.glh"

$MAX_LIGHTS

out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// Mesh's global material
uniform vec3 Kd;
uniform float metallic;
uniform float shininess;
uniform uint texMask;

// Texture units 0-3
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D shininessMap;
uniform sampler2D metallicMap;

// Lights
uniform vec4 lightVectors[MAX_LIGHTS]; // position or direction
uniform vec3 emissions[MAX_LIGHTS];
uniform mat4 lightTransforms[MAX_LIGHTS];
uniform sampler2D shadowMaps[MAX_LIGHTS]; // units 8->
uniform samplerCube shadowCubeMaps[MAX_LIGHTS]; // units 8 + MAX_LIGHTS ->
uniform uint nLights;

// Other parameters
uniform vec3 cameraPos;
uniform bool useVSM;
uniform float svmBleedFix;

void main() {
    vec3 albedo = Kd;
	float alpha = 1.0 - shininess; // alpha = roughness
	float metallic = metallic;
	vec3 N = normalize(Normal);
	vec3 V = normalize(cameraPos - WorldPos);
    
    READ_PBR_TEXTURES(TexCoords)

	// Metallic workflow: use albedo color as F0
	vec3 F0 = vec3(0.04); // percentage of light reflected at normal incidence
	F0 = mix(F0, albedo, metallic);
	
	vec3 Lo = vec3(0.0);
	for (uint i = 0U; i < nLights; i++) {
		vec3 L, radiance;
		vec4 lightVec = lightVectors[i];

		float shadow = 0.0;
		if (lightVec.w == 0.0) {
			L = -1.0 * normalize(vec3(lightVec));
			radiance = emissions[i];
			vec4 posLightSpace = lightTransforms[i] * vec4(WorldPos, 1.0);
			shadow = (useVSM) ? checkShadowDepthDirVSM(shadowMaps[i], posLightSpace, svmBleedFix)
							  : checkShadowDepthDir(shadowMaps[i], posLightSpace, dot(N, L));
		}
		else {
			vec3 lightPos = vec3(lightVec);
			vec3 toLight = lightPos - WorldPos;
			float dist = length(toLight);
			radiance = emissions[i] / (dist * dist);
			L = normalize(toLight);
			shadow = (useVSM) ? checkShadowDepthPointVSM(shadowCubeMaps[i], -toLight, svmBleedFix)
							  : checkShadowDepthPoint(shadowCubeMaps[i], -toLight, N);
		}
		
		vec3 H = normalize(L + V);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
		vec3 bsdfSpec = evalGGXReflect(alpha, F, N, L, V);
		vec3 bsdfDiff = albedo / PI;
		float NdotL = max(dot(N, L), 0.0);

		// Add contribution
		vec3 bsdf = bsdfSpec + (1.0 - F) * (1.0 - metallic) * bsdfDiff; // bsdfSpec contains F
		Lo += (1.0 - shadow) * bsdf * radiance * NdotL;
	}

	// Ambient
	Lo += vec3(0.025 * albedo / PI);

	// Tone mapping
	Lo = Lo / (Lo + vec3(1.0));
	vec3 shading = pow(Lo, vec3(1.0/2.2)); // Gamma-correct
    FragColor = vec4(shading, 1.0);
}