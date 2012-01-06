#version 120

// Kernel
void main(void)
{

	// Transform position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

}
