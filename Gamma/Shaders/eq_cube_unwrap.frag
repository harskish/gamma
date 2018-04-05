#version 330
out vec4 FragColor;
in vec3 posWorld;

uniform sampler2D equirecTex;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleEquirec(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    // Compute direction in world space (assume camera at origin)
	vec2 uv = SampleEquirec(normalize(posWorld));
    vec3 color = texture(equirecTex, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
