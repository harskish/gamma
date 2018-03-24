#version 330 core

in vec4 FragPos;
out vec4 FragColor;

uniform vec3 lightPos;
uniform float farPlane;
uniform bool useVSM;

// Handles both normal and VSM shadows based on uniform toggle
void main()
{
    // get distance between fragment and light source
    float lightDistance = length(FragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / farPlane;
    
	if (useVSM) {
		FragColor = vec4(lightDistance, lightDistance * lightDistance, 0.0, 0.0);
	}
	else {
		gl_FragDepth = lightDistance;
	}
} 