#version 330

out vec4 FragColor;
in vec2 TexCoords;
  
uniform sampler2D sourceTexture;
uniform vec2 invRes;
uniform bool useKarisAvg;

float luma(vec4 color) {
	return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

// Box filter w/ Karis avg
vec4 avg4Karis(vec4 p0, vec4 p1, vec4 p2, vec4 p3) {
	vec4 sum = vec4(0.0);
	sum += (1.0 / (1.0 + luma(p0))) * p0;
	sum += (1.0 / (1.0 + luma(p1))) * p1;
	sum += (1.0 / (1.0 + luma(p2))) * p2;
	sum += (1.0 / (1.0 + luma(p3))) * p3;
	return vec4(sum.rgb / sum.w, 1.0);
}

// Standard box filter
vec4 avg4(vec4 p0, vec4 p1, vec4 p2, vec4 p3) {
	return (p0 + p1 + p2 + p3) * 0.25;
}

// Downsampling filter w/ partial Karis average
// AA applied only at MIP0 -> MIP1 transition
// For more details see presentation:
// "Next Generation Post Processing in Call of Duty Advanced Warfare" by Jorge Jimenez
void main() {
	vec2 offset = invRes;
        
	vec4 c0 = texture(sourceTexture, TexCoords + vec2(-2, -2) * offset);
	vec4 c1 = texture(sourceTexture, TexCoords + vec2(0, -2)*offset);
	vec4 c2 = texture(sourceTexture, TexCoords + vec2(2, -2) * offset);
	vec4 c3 = texture(sourceTexture, TexCoords + vec2(-1, -1) * offset);
	vec4 c4 = texture(sourceTexture, TexCoords + vec2(1, -1) * offset);
	vec4 c5 = texture(sourceTexture, TexCoords + vec2(-2, 0) * offset);
	vec4 c6 = texture(sourceTexture, TexCoords);
	vec4 c7 = texture(sourceTexture, TexCoords + vec2(2, 0) * offset);
	vec4 c8 = texture(sourceTexture, TexCoords + vec2(-1, 1) * offset);
	vec4 c9 = texture(sourceTexture, TexCoords + vec2(1, 1) * offset);
	vec4 c10 = texture(sourceTexture, TexCoords + vec2(-2, 2) * offset);
	vec4 c11 = texture(sourceTexture, TexCoords + vec2(0, 2) * offset);
	vec4 c12 = texture(sourceTexture, TexCoords + vec2(2, 2) * offset);

	vec4 color;
	if (useKarisAvg) {
		color = avg4Karis(c0, c1, c5, c6) * 0.125 +
				avg4Karis(c1, c2, c6, c7) * 0.125 +
				avg4Karis(c5, c6, c10, c11) * 0.125 +
				avg4Karis(c6, c7, c11, c12) * 0.125 +
				avg4Karis(c3, c4, c8, c9) * 0.5;
	}
	else {
		color = avg4(c0, c1, c5, c6) * 0.125 +
				avg4(c1, c2, c6, c7) * 0.125 +
				avg4(c5, c6, c10, c11) * 0.125 +
				avg4(c6, c7, c11, c12) * 0.125 +
				avg4(c3, c4, c8, c9) * 0.5;
	}

	FragColor = color;
}