#version 330

#include "ggx_funcs.glh"
#include "shadow_funcs.glh"

$MAX_LIGHTS

const uint DIFFUSE_MASK = (1U << 0);
const uint NORMAL_MASK = (1U << 1);
const uint SHININESS_MASK = (1U << 2);
const uint ROUGHNESS_MASK = (1U << 3);
const uint METALLIC_MASK = (1U << 4);
const uint BUMP_MASK = (1U << 5);
const uint DISPLACEMENT_MASK = (1U << 6);
const uint EMISSION_MASK = (1U << 7);

const float PI = 3.14159265359;

out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform vec3 cameraPos;

// Mesh's global material
uniform vec3 Kd;
uniform float metallic;
uniform float shininess;
uniform uint texMask;

// Override parameters per fragment
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
uniform bool useVSM;
uniform float svmBleedFix;


// Create tangent base on the fly
vec3 worldSpaceNormal() {
	vec3 Nt = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

	vec3 Q1 = dFdx(WorldPos);
	vec3 Q2 = dFdy(WorldPos);
	vec2 st1 = dFdx(TexCoords);
	vec2 st2 = dFdy(TexCoords);
 
	vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	vec3 B = normalize(-Q1*st2.s + Q2*st1.s);
	mat3 TBN = mat3(T, B, Normal);

	return normalize(TBN * Nt);
}

// Schlick's approximation
vec3 fresnelSchlick(float cosTh, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTh, 5.0);
}

void main() {
    vec3 albedo = Kd;
	float alpha = 1.0 - shininess; // alpha = roughness
	float metallic = metallic;
	vec3 N = normalize(Normal);
	vec3 V = normalize(cameraPos - WorldPos);
    
    if ((texMask & DIFFUSE_MASK) != 0U)
        albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)); // convert to linear space
	if ((texMask & NORMAL_MASK) != 0U)
        N = worldSpaceNormal();
	if ((texMask & SHININESS_MASK) != 0U)
        alpha = 1.0 - texture(shininessMap, TexCoords).r;
	if ((texMask & ROUGHNESS_MASK) != 0U)
        alpha = texture(shininessMap, TexCoords).r;
	if ((texMask & METALLIC_MASK) != 0U)
        metallic = texture(metallicMap, TexCoords).r;

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