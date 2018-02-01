#version 150

uniform sampler2D uTex0; // new bus texture
uniform sampler2D uTex1; // mask fbo

in vec2 TexCoord0;
out vec4 oColor;

void main(void) {
	oColor = texture(uTex0, TexCoord0);
	oColor.a *= 1 - texture(uTex1, TexCoord0).r;
}
