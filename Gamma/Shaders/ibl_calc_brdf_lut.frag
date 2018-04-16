#version 330

#include "random.glh"
#include "ggx_funcs.glh"

in vec2 TexCoords;
out vec2 FragColor;

float GeometrySchlickGGX(float NdotV, float alpha) {
    float k = (alpha * alpha) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float alpha) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, alpha) * GeometrySchlickGGX(NdotL, alpha);
} 

void main() {
	float NdotV = TexCoords.x;
	float roughness = TexCoords.y;
    
	vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV); // = sin
    V.y = 0.0;
    V.z = NdotV; // = cos

    float A = 0.0; // scale
    float B = 0.0; // bias

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; i++) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = ggxSampleLobe(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness); // ggxG(roughness, V, L, N, H);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
	
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
	
	FragColor = vec2(A, B);
}