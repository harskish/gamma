#version 330

in vec3 velocity;
in vec3 position;
in vec3 posEye; // position of center in eye space

out vec4 color;

void main() {
    const float shininess = 40.0;

    // calculate normal from texture coordinates
    vec3 n;
    n.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(n.xy, n.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
    n.z = sqrt(1.0-mag);

    // calculate lighting
    vec3 lightDir = normalize(vec3(0.0f, 2.0f, 2.0f) - position);
    float diffuse = max(0.0, dot(lightDir, n));
    vec3 tempColor = diffuse * vec3(0.9, 0.94, 1.0);

    color = vec4(tempColor + 0.1f, 1.0f);
}
