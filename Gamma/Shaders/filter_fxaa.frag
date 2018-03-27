#version 330

/*
 * Fast Approximate Anti-Aliasing
 * Based on http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
 */

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform vec2 invTexSize;
uniform float fxaaSpanMax;   // 8.0
uniform float fxaaReduceMin; // 1.0/128.0
uniform float fxaaReduceMul; // 1.0/8.0

// Optimized luma, sacrifices pure-blue aliasing detection
// Should become a single FMA
float FxaaLuma(vec3 rgb) {
	return rgb.y * (0.587/0.299) + rgb.x;
}

void main() {
	float lumaNW = FxaaLuma(texture(sourceTexture, TexCoords + (vec2(-1.0, -1.0) * invTexSize)).xyz);
	float lumaNE = FxaaLuma(texture(sourceTexture, TexCoords + (vec2( 1.0, -1.0) * invTexSize)).xyz);
	float lumaSW = FxaaLuma(texture(sourceTexture, TexCoords + (vec2(-1.0,  1.0) * invTexSize)).xyz);
	float lumaSE = FxaaLuma(texture(sourceTexture, TexCoords + (vec2( 1.0,  1.0) * invTexSize)).xyz);
	float lumaM = FxaaLuma(texture(sourceTexture, TexCoords).xyz);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	// Calculate blur direction based on neighborhood luma
	// Implicitly does edge detection (blur dir becomes zero)
	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	// Determine blur magnitude (normalize min component)
	// Avoid div. by zero by adding bias
	float dirReduce = max(
            (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * fxaaReduceMul),
            fxaaReduceMin);
	float invDirAdj = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

	// Scale and transform dir into texel space
	vec2 maxSpan = vec2(fxaaSpanMax, fxaaSpanMax);
	dir = clamp(dir * invDirAdj, -maxSpan, maxSpan) * invTexSize;

	// Perform blurs with two distances
	vec3 rgbA = (1.0/2.0) * (
                texture(sourceTexture, TexCoords + dir * (1.0/3.0 - 0.5)).xyz +
                texture(sourceTexture, TexCoords + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
                texture(sourceTexture, TexCoords + dir * (0.0/3.0 - 0.5)).xyz +
                texture(sourceTexture, TexCoords + dir * (3.0/3.0 - 0.5)).xyz);
	float lumaB = FxaaLuma(rgbB);

	// Check luma range of larger blur
	// Outside of range indicates included non-edge texels
	FragColor = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? vec4(rgbA, 1.0) : vec4(rgbB, 1.0);
}