// Adding the word "compatibility" let's us use 
// legacy built-in uniforms
#version 150 compatibility

// Uniforms
uniform mat4 mvp;
uniform sampler2D positions;

// Output attributes
out vec4 vertex;
out vec4 texCoord;

// Kernel
void main(void)
{

	// Set output attributes
	texCoord = gl_MultiTexCoord0;
	vertex = texture2D(positions, texCoord.st);

	// Set position
	gl_Position = vertex;

}
