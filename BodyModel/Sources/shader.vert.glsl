#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec3 position;
out vec2 texCoord;
out vec3 normal;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void kore() {
	vec4 newPos = M * vec4(pos, 1.0);
	gl_Position = P * V * newPos;
	position = newPos.xyz / newPos.w;
	texCoord = tex;
	normal = (V * M * vec4(nor, 0.0)).xyz;
}
