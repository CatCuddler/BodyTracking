#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec3 eyeCoord;
out vec3 lightDirection;
out vec2 texCoord;
out vec3 normal;
out vec3 diffuseCol;
out vec3 specularCol;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 lightPos;

void kore() {
	// Pass some variables to the fragment shader
	eyeCoord = (V * M * vec4(pos, 1.0)).xyz;
	
	vec3 transformedLightPos = (V * M * vec4(lightPos, 1.0)).xyz;
	lightDirection = transformedLightPos - eyeCoord;
	
	texCoord = tex;
	
	mat4 N = transpose(inverse(mat4(M)));
	normal = (N * vec4(nor, 0.0)).xyz;
	
	diffuseCol = diffuse;
	specularCol = specular;
	
	// Apply all matrix transformations to vert
	gl_Position = P * vec4(eyeCoord, 1.0);
}
