#version 120

// Uniforms
uniform float brightTolerance;
uniform float depth;
uniform float height;
uniform mat4 mvp;
uniform sampler2D positions;
uniform vec3 scale;
uniform bool transform;
uniform float width;

// Output attributes
varying vec4 texCoord;
//varying vec4 vertex;

// Kernel
void main(void)
{

	// Set output attributes
	texCoord = gl_MultiTexCoord0;
	//vertex = gl_Vertex;
	
	// Set position
	gl_Position = mvp * gl_Vertex;;

}
