#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

varying vec3 position;
varying vec2 texCoord;
varying vec3 normal;

void kore() {
	gl_FragColor = texture2D(tex, texCoord);
	//gl_FragColor = vec4(((texture2D(tex, texCoord) * (0.8 + max(dot(vec3(0, 1, 0), normal), 0.0) * 0.2)).xyz), 1);
}
