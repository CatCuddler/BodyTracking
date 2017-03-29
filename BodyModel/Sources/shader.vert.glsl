attribute vec3 pos;
attribute vec2 tex;
attribute vec3 nor;

varying vec3 position;
varying vec2 texCoord;
varying vec3 normal;

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
