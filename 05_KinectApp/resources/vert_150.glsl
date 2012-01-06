// Adding the word "compatibility" let's us use 
// legacy built-in uniforms
#version 150 compatibility

// Input attributes
in vec4 vertexIn;

// Output attributes
out vec4 texCoord;
out vec4 vertex;

// Kernel
void main(void)
{

	// Set output attributes
	texCoord = gl_MultiTexCoord0;
	vertex = vec4(vertexIn);
	
	// Set position
	gl_Position = vertex;

}
