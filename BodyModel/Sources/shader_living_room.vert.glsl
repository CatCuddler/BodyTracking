#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec2 texCoord;
out vec3 normal;
out vec3 lightDirection;
out vec3 eyeCoord;
out vec4 tintCol;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform mat4 N;
uniform vec4 tint;
uniform vec3 lightPos;

void kore() {
	eyeCoord = (V * M * vec4(pos, 1.0)).xyz;
	vec3 transformedLightPos = (V * M * vec4(lightPos, 1.0)).xyz;
	lightDirection = transformedLightPos - eyeCoord;
	
	gl_Position = P * vec4(eyeCoord.x, eyeCoord.y, eyeCoord.z, 1.0);
	texCoord = tex;
	normal = (V * M * vec4(nor, 0.0)).xyz;
	//normal = (N * vec4(nor, 0.0)).xyz;
	tintCol = tint;
}
