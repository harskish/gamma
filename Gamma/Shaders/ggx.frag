#version 330

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
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D shininessMap;
uniform sampler2D metallicMap;


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

// Unidirectional shadowing-masking function
// G1(v, m) = 2 / 1 + sqrt( 1 + a^2 * tan^2v )  (eg. 34)
float ggxG1(float alpha, vec3 v, vec3 n, vec3 m) {
	float mDotV = dot(m,v);
	float nDotV = dot(n,v);
	
	// Check sidedness agreement (eq. 7)
	if (nDotV * mDotV <= 0.0)
		return 0.0;

	// tan^2 = sin^2 / cos^2
	float cosThSq = nDotV * nDotV;
	float tanSq = (cosThSq > 0.0) ? ((1.0 - cosThSq) / cosThSq) : 0.0;
    return 2.0 / (1.0 + sqrt(1.0 + alpha * alpha * tanSq));
}

// Smith approximation for G:
// Return product of the unidirectional masking functions
float ggxG(float alpha, vec3 dirIn, vec3 dirOut, vec3 n, vec3 m) {
	return ggxG1(alpha, dirIn, n, m) * ggxG1(alpha, dirOut, n, m);
}

// Microfacet distribution function (GTR2)
// D(m) = a^2 / PI * cosT^4 * (a^2 + tanT^2)^2  (eq. 33)
float ggxD(float alpha, vec3 n, vec3 m) {
	float nDotM = dot(n, m);

	if (nDotM <= 0.0)
		return 0.0;

	// tan^2 = sin^2 / cos^2
	float nDotMSq = nDotM * nDotM;
	float tanSq = nDotM != 0.0 ? ((1.0 - nDotMSq) / nDotMSq) : 0.0;

	float aSq = alpha * alpha;
	float denom = PI * nDotMSq * nDotMSq * (aSq + tanSq) * (aSq + tanSq);
	return denom > 0.0 ? (aSq / denom) : 0.0;
}

// dirIn points outwards
vec3 evalGGXReflect(float alpha, vec3 F, vec3 N, vec3 dirIn, vec3 dirOut) {
	// Calculate halfway vector
	vec3 H = normalize(dirIn + dirOut);

	float iDotN = dot(dirIn, N);
	float oDotN = dot(dirOut, N);

	// Evaluate BSDF (eq. 20)
	vec3 Ks = vec3(1.0);
	float D = ggxD(alpha, N, H);
	float G = ggxG(alpha, dirIn, dirOut, N, H);
	float den = (4.0 * iDotN * oDotN);
	return (den != 0.0) ? (Ks * F * G * D / den) : vec3(0.0, 0.0, 0.0);
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
	
	const int LIGHTS = 4;
	vec3 pointLights[LIGHTS];
	pointLights[0] = vec3(1.0, 1.0, 2.0);
	pointLights[1] = vec3(-1.0, 1.0, 2.0);
	pointLights[2] = vec3(1.0, -1.0, 2.0);
	pointLights[3] = vec3(-1.0, -1.0, 2.0);

	// Metallic workflow: use albedo color as F0
	vec3 F0 = vec3(0.04); // percentage of light reflected at normal incidence
	F0 = mix(F0, albedo, metallic);
	
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < LIGHTS; i++) {
		vec3 lightPos = pointLights[i];
		vec3 emission = vec3(5.0);
		float dist = length(lightPos - WorldPos);
		vec3 radiance = emission / (dist * dist);

		vec3 L = normalize(lightPos - WorldPos);
		vec3 H = normalize(L + V);

		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
		vec3 bsdfSpec = evalGGXReflect(alpha, F, N, L, V);
		vec3 bsdfDiff = albedo / PI;

		vec3 bsdf = bsdfSpec + (1.0 - F) * (1.0 - metallic) * bsdfDiff; // bsdfSpec contains F
		float NdotL = max(dot(N, L), 0.0);

		Lo += bsdf * radiance * NdotL;
	}

	// Ambient
	Lo += vec3(0.025 * albedo / PI);

	// Tone mapping
	Lo = Lo / (Lo + vec3(1.0));
	vec3 shading = pow(Lo, vec3(1.0/2.2)); // Gamma-correct
    FragColor = vec4(shading, 1.0);
}