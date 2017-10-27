#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec3 eyeCoord;
out vec2 texCoord;
out vec3 normal;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 MInverse;

void kore() {
	// Pass some variables to the fragment shader
	eyeCoord = (M * vec4(pos, 1.0)).xyz;
	texCoord = tex;
	normal = normalize((transpose(mat4(MInverse)) * vec4(nor, 0.0)).xyz);
	
	// Apply all matrix transformations to vert
	gl_Position = P * V * M * vec4(pos, 1.0);
}