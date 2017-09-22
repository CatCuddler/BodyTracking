#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec3 position;
out vec2 texCoord;
out vec3 normal;
out vec3 lightDirection;
out vec3 eyeCoord;
out vec4 tintCol;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 tint;

void kore() {
	vec3 lightPos = vec3(0.0, 0.0, 5.0);
	
	eyeCoord = (V * M * vec4(pos, 1.0)).xyz;
	vec3 transformedLightPos = (V * M * vec4(lightPos, 1.0)).xyz;
	lightDirection = transformedLightPos - eyeCoord;
	
	gl_Position = P * vec4(eyeCoord.x, eyeCoord.y, eyeCoord.z, 1.0);
	texCoord = tex;
	normal = (V * M * vec4(nor, 0.0)).xyz;
	tintCol = tint;
}
