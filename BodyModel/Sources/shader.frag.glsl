#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

varying vec3 position;
varying vec2 texCoord;
varying vec3 normal;

void kore() {
	gl_FragColor = texture2D(tex, texCoord);
}