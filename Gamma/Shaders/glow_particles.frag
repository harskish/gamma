#version 330

in vec3 velocity;
in vec3 position;
in vec3 posEye; // position of center in eye space

out vec4 color;

uniform vec3 sunPosition;

void main() {
    // Normal from texture coordinates
    vec3 N;
    N.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard; // outside circle
    N.z = sqrt(1.0-mag);

    // Calculate lighting
    // vec3 lightDir = normalize(sunPosition - position);
    // vec3 direct = 100.0 * vec3(0.8) * max(0.0, dot(lightDir, N));
    // vec3 ambient = vec3(0.2);
    //
    //color = vec4(ambient + direct, 1.0);

    // Bright glowing dots
    vec3 glow = 3.0 * vec3(1, 0.905, 0.160) + 1e-8 * sunPosition;
    color = vec4(glow, 1.0);
}
