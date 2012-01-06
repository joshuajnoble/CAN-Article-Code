#version 120

// Uniforms
uniform sampler2D tex;
uniform bool transform;

// Input attributes
varying vec2 uv;

// Kernel
void main(void)
{

	// Pull color from image or draw white if not transforming
	gl_FragColor = transform ? texture2D(tex, uv.st) : vec4(1.0, 1.0, 1.0, 1.0);

}
