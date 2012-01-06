#version 120

// Output attributes
varying vec4 uv;

// Kernel
void main()
{

	// Set properties
	uv = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

}
