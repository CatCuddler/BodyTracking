#version 450

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

in vec3 position;
in vec2 texCoord;
in vec3 normal;

out vec4 FragColor;

void kore() {
	FragColor = texture(tex, texCoord);
	//gl_FragColor = vec4(((texture2D(tex, texCoord) * (0.8 + max(dot(vec3(0, 1, 0), normal), 0.0) * 0.2)).xyz), 1);
}
