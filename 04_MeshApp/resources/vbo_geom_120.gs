#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable

// Uniforms
uniform mat4 mvp;
uniform vec2 pixel;
uniform sampler2D positions;
uniform bool transform;

// Input attributes
varying in vec4 vertex[1]; // we're loading points
varying in vec4 texCoord[1]; // we're loading points

// Output attributes
varying vec4 gsNormal;
varying vec4 gsPosition;
varying vec4 gsuv;

// Adds a vertex to the current primitive
void addVertex(vec4 vert, vec4 norm)
{

	// Assign values to output attributes
	gsNormal = norm;
	gl_Position = vert;

	// Create vertex
	EmitVertex();

}

// Kernel
void main(void)
{

	// Passes original vertex and texture coordinate to fragment shader
	gsPosition = vertex[0];
	gsuv = texCoord[0];

	// Transform point to box
	if (transform)
	{

		// Find corners of quad
		vec4 vert0 = mvp * gsPosition;
		vec4 vert1 = mvp * texture2D(positions, gsuv.st + vec2(pixel.x, 0.0));
		vec4 vert2 = mvp * texture2D(positions, gsuv.st + pixel);
		vec4 vert3 = mvp * texture2D(positions, gsuv.st + vec2(0.0, pixel.y));

		// Calculate normals
		vec4 norm0 = vec4(normalize(cross(vec3(vert1.xyz - vert0.xyz), vec3(vert1.xyz - vert3.xyz))), 0.0);
		vec4 norm1 = vec4(normalize(cross(vec3(vert2.xyz - vert1.xyz), vec3(vert2.xyz - vert3.xyz))), 0.0);

		// Left triangle
		addVertex(vert0, norm0);
		addVertex(vert3, norm0);
		addVertex(vert1, norm0);
		EndPrimitive();

		// Right triangle
		addVertex(vert1, norm1);
		addVertex(vert3, norm1);
		addVertex(vert2, norm1);
		EndPrimitive();

	}
	else
	{

		// Pass-thru
		gl_Position = mvp * vertex[0];
		EmitVertex();
		EndPrimitive();

	}

}
