#version 330

const uint DIFFUSE_MASK = (1U << 0);
const uint NORMAL_MASK = (1U << 1);
const uint SHININESS_MASK = (1U << 2);
const uint METALLIC_MASK = (1U << 3);
const uint BUMP_MASK = (1U << 4);
const uint DISPLACEMENT_MASK = (1U << 5);
const uint EMISSION_MASK = (1U << 6);

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

void main() {
    vec3 color = Kd;
    
    if ((texMask & DIFFUSE_MASK) != 0U) {
        color = texture(albedoMap, TexCoords).rgb;
    }

    vec3 lightPos = cameraPos;

    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - WorldPos);
    vec3 L = normalize(lightPos - WorldPos);
    vec3 R = reflect(-V, N);
    vec3 H = normalize(L + V);

    vec3 emission = vec3(1.0);

    vec3 Ks = vec3(0.5);
    vec3 diffuse = emission * max(0.0, dot(N, V));
    
    
    // Blinn-Phong
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = Ks * spec * emission;
    
    
    vec3 ambient = vec3(0.1);

    vec3 shading = (diffuse + specular + ambient) * color;
    FragColor = vec4(shading, 1.0);
}