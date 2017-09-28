#version 450

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

uniform vec3 light;
uniform vec3 eye;

uniform float spec;
uniform float roughness;
uniform int mode;

in vec3 position;
in vec2 texCoord;
in vec3 normal;

out vec4 FragColor;

// Schlick's Fresnel approximation
float F(vec3 l, vec3 h) {
	return spec + (1.0 - spec) * pow((1.0 - dot(l, h)), 5.0);
}

// Trowbridge-Reitz normal distribution term
float D(vec3 n, vec3 h) {
	float pi = 3.14159265358979323846264;
	return roughness * roughness / (pi * pow(dot(n, h) * dot(n, h) * (roughness * roughness - 1.0) + 1.0, 2.0));
}

// Cook and Torrance's geometry factor
float G(vec3 l, vec3 v, vec3 h, vec3 n) {
	float term1 = (2.0 * dot(n, h) * dot(n, v)) / dot(v, h);
	float term2 = (2.0 * dot(n, h) * dot(n, l)) / dot(v, h);
	return min(1.0, min(term1, term2));
}

void main() {
	// determine the normal vector
	vec3 n = normalize(normal);
	// determine the light vector
	vec3 l = light - position;
	l = normalize(l);
	// determine the view vector
	vec3 v = position - eye;;
	v = normalize(v);
	// determine the half vector
	vec3 h = l + v;
	h = normalize(h);
	// determine the BRDF
	float f = (F(l, h) * G(l, v, h, n) * D(n, h)) / (4.0 * dot(n, l) * dot(n, v));
	// determine texel
	vec3 t = pow(texture(tex, texCoord).rgb, vec3(2.2));
	
	// determine view dependend on mode
	vec3 rgb;
	if (mode == 1) {
		rgb = F(l, h) * vec3(1);
	}
	else if (mode == 2) {
		rgb = D(n, h) * vec3(1);
	}
	else if (mode == 3) {
		rgb = G(l, v, h, n) * vec3(1);
	}
	else {
		rgb = f * dot(l, n) * vec3(1) + dot(l, n) * t;
	}
	
	// other views
	//float fpure = 1.0 / (4.0 * dot(n, l) * dot(n, v));
	//vec3 rgb = fpure * texture2D(tex, texCoord).rgb;
	//vec3 rgb = t * dot(l, n);
	//vec3 rgb = (n + 1.0) / 2.0;

	FragColor = vec4(pow(rgb, vec3(1.0 / 2.2)), 1);
}
