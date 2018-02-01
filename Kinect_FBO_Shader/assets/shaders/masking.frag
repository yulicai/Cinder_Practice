#version 150

uniform sampler2D uTex0; // source FBO
// uniform sampler2D uTex1; // brush texture
uniform vec2 uPos; // each blob center position
uniform float uBlobRadius; // each blob radius
uniform vec2 uSize;  // size of canvas, in cpu

in vec2 TexCoord0;
out vec4 oColor;

float circle(in vec2 _st, in float _radius, vec2 _center) {
	vec2 dist = _st - _center;
	return 1. - smoothstep(_radius - (_radius*0.07),
		_radius + (_radius*0.07),
		dot(dist, dist)*4.0);
}

	vec2 convertedBlogCenter = vec2((uPos.x / uSize.x), (1.0 - uPos.y / uSize.y)*1.1);

void main(void) {
	vec2 st = gl_FragCoord.xy / uSize.xy;

	// FOR THE ASPECT RATIO
	if (uSize.y > uSize.x) {
	st.y *= uSize.y / uSize.x;
	st.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;

	convertedBlogCenter.y *= uSize.y / uSize.x;
	convertedBlogCenter.y -= (uSize.y*.5 - uSize.x*.5) / uSize.x;
	}
	else {
	st.x *= uSize.x / uSize.y;
	st.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
	convertedBlogCenter.x *= uSize.x / uSize.y;
	convertedBlogCenter.x -= (uSize.x*.5 - uSize.y*.5) / uSize.y;
	}

	// oColor = texture(uTex0, TexCoord0) + texture(uTex1, uPos + TexCoord0) * 0.8;
	float tempCircle = circle(st, uBlobRadius,convertedBlogCenter);
	oColor = texture(uTex0, TexCoord0) + vec4(vec3(tempCircle),1.0);
	oColor.a = 1;
}
