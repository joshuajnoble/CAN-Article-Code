#version 120

// Uniforms
uniform mat4 mvp;
uniform sampler2D positions;

// Output attributes
varying vec4 vertex;
varying vec4 texCoord;

// Kernel
void main(void)
{

	// Set output attributes
	texCoord = gl_MultiTexCoord0;
	vertex = texture2D(positions, texCoord.xy);

	// Set position
	gl_Position = vertex;

}
