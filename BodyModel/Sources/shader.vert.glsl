#version 450

in vec3 pos;
in vec2 tex;
in vec3 nor;

out vec2 texCoord;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void kore() {
	gl_Position = P * V * M * vec4(pos, 1.0);
	texCoord = tex;
}
