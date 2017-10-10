#version 450

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

uniform vec3 diffuseCol;
uniform vec3 specularCol;
uniform float specularPow;

#define MAX_LIGHTS 10
uniform int numLights;
uniform vec4 lightPos[MAX_LIGHTS];

in vec2 texCoord;
in vec3 normal;
in vec3 lightDirection;
in vec3 eyeCoord;

out vec4 FragColor;

vec3 applyLight(vec4 lightPosition) {
	
	vec3 lightDirection;
	float attenuation = 1.0;
	
	if (lightPosition.w == 0.0) {
		// Spot light
		lightDirection = normalize(lightPosition.xyz - eyeCoord);
		
		float distanceToLight = length(lightPosition.xyz - eyeCoord);
		float lightAttenuation = 0.1;
		attenuation = 1.0 / (1.0 + lightAttenuation * pow(distanceToLight, 2));
		
		// Cone restrictions (affects attenuation)
		vec3 coneDirection = vec3(0, -1, 0);
		float coneAngle = 15.0;
		float lightToSurfaceAngle = degrees(acos(dot(-lightDirection, normalize(coneDirection))));
		if(lightToSurfaceAngle > coneAngle){
			attenuation = 0.0;
		}
		
	} else {
		// Directional light
		lightDirection = normalize(lightPosition.xyz);
		attenuation = 1.0; // No attenuation for directional lights
	}
	
	// Ambient
	const float amb = 0.3;
	vec3 ambient = vec3(amb);
	
	// Diffuse
	vec3 diffuse = max(dot(lightDirection, normal), 0.0) * vec3(diffuseCol);
	
	// Specular
	vec3 halfVector = normalize(lightDirection - normalize(eyeCoord));
	vec3 specular = pow(max(0.0, dot(halfVector, reflect(-lightDirection, normal))), specularPow) * vec3(specularCol);
	
	vec3 light = ambient + attenuation * (diffuse + specular);
	return light;
}

void main() {
	
	vec3 finalLight = vec3(0, 0, 0);
	for (int i = 0; i < numLights; ++i) {
		finalLight += applyLight(lightPos[i]);
	}
	
	FragColor = vec4(finalLight, 1.0) * texture(tex, texCoord);
}
