// Adding the word "compatibility" let's us use 
// legacy built-in uniforms
#version 150 compatibility

// Input attributes
in vec4 vertex;

// Output attributes
out vec4 texCoord;

// Kernel
void main()
{

	// Set properties
	texCoord = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;

}
