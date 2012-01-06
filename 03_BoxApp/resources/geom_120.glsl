#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable 

// Uniforms
uniform vec4 dimensions;
uniform mat4 mvp;
uniform float rotation;
uniform bool transform;

// Input attributes
varying vec4 vertex;

// Output attributes
varying vec4 normal;
varying vec4 position;
varying vec2 uv;

// Adds a vertex to the current primitive
void addVertex(vec2 texCoord, vec4 vert, vec4 norm)
{

	// Assign values to output attributes
	normal = norm;
	uv = texCoord;
	gl_Position = gl_PositionIn[0] * vert;

	// Create vertex
	EmitVertex();

}

// Kernel
void main(void)
{

	// Passes original vertex position to fragment shader
	position = vertex;

	// Transform point to boz
	if (transform)
	{

		// Use Y position for phase
		float angle = vertex.y * rotation;
	
		// Define rotation matrix
		mat4 rotMatrix;
		rotMatrix[0].x = 1.0 + cos(angle);
		rotMatrix[0].y = 1.0 + -sin(angle);
		rotMatrix[0].z = 1.0 + -rotMatrix[0].x;
		rotMatrix[0].w = 0.0;
		rotMatrix[1].x = 1.0 + -rotMatrix[0].z;
		rotMatrix[1].y = 1.0 + -rotMatrix[0].y;
		rotMatrix[1].z = 1.0 + -rotMatrix[0].x;
		rotMatrix[1].w = 0.0;
		rotMatrix[2].x = 0.0;
		rotMatrix[2].y = 0.0;
		rotMatrix[2].z = 1.0;
		rotMatrix[2].w = 0.0;
		rotMatrix[3].x = 0.0;
		rotMatrix[3].y = 0.0;
		rotMatrix[3].z = 0.0;
		rotMatrix[3].w = 1.0;

		// Define vertices
		vec4 vert0 = mvp * (vertex + vec4(dimensions * vec4(-1.0, -1.0, -1.0, 0.0)) * rotMatrix); // 0 ---
		vec4 vert1 = mvp * (vertex + vec4(dimensions * vec4( 1.0, -1.0, -1.0, 0.0)) * rotMatrix); // 1 +--
		vec4 vert2 = mvp * (vertex + vec4(dimensions * vec4(-1.0,  1.0, -1.0, 0.0)) * rotMatrix); // 2 -+-
		vec4 vert3 = mvp * (vertex + vec4(dimensions * vec4( 1.0,  1.0, -1.0, 0.0)) * rotMatrix); // 3 ++-
		vec4 vert4 = mvp * (vertex + vec4(dimensions * vec4(-1.0, -1.0,  1.0, 0.0)) * rotMatrix); // 4 --+
		vec4 vert5 = mvp * (vertex + vec4(dimensions * vec4( 1.0, -1.0,  1.0, 0.0)) * rotMatrix); // 5 +-+
		vec4 vert6 = mvp * (vertex + vec4(dimensions * vec4(-1.0,  1.0,  1.0, 0.0)) * rotMatrix); // 6 -++
		vec4 vert7 = mvp * (vertex + vec4(dimensions * vec4( 1.0,  1.0,  1.0, 0.0)) * rotMatrix); // 7 +++

		// Define normals
		vec4 norm0 = mvp * vec4( 1.0,  0.0,  0.0, 0.0); // Right
		vec4 norm1 = mvp * vec4( 0.0,  1.0,  0.0, 0.0); // Top
		vec4 norm2 = mvp * vec4( 0.0,  0.0,  1.0, 0.0); // Front
		vec4 norm3 = mvp * vec4(-1.0,  0.0,  0.0, 0.0); // Left
		vec4 norm4 = mvp * vec4( 0.0, -1.0,  0.0, 0.0); // Bottom
		vec4 norm5 = mvp * vec4( 0.0,  0.0, -1.0, 0.0); // Back

		// Define UV coordinates
		vec2 uv0 = vec2(0.0, 0.0); // --
		vec2 uv1 = vec2(1.0, 0.0); // +-
		vec2 uv2 = vec2(1.0, 1.0); // ++
		vec2 uv3 = vec2(0.0, 1.0); // -+

		// Left
		addVertex(uv1, vert5, norm3);
		addVertex(uv2, vert7, norm3);
		addVertex(uv0, vert4, norm3);
		addVertex(uv3, vert6, norm3);
		EndPrimitive();

		// Right
		addVertex(uv3, vert3, norm0);
		addVertex(uv0, vert1, norm0);
		addVertex(uv2, vert2, norm0);
		addVertex(uv1, vert0, norm0);
		EndPrimitive();

		// Top
		addVertex(uv1, vert7, norm1);
		addVertex(uv2, vert3, norm1);
		addVertex(uv0, vert6, norm1);
		addVertex(uv3, vert2, norm1);
		EndPrimitive();

		// Bottom
		addVertex(uv1, vert1, norm4);
		addVertex(uv2, vert5, norm4);
		addVertex(uv0, vert0, norm4);
		addVertex(uv3, vert4, norm4);
		EndPrimitive();

		// Front
		addVertex(uv2, vert3, norm2);
		addVertex(uv3, vert7, norm2);
		addVertex(uv1, vert1, norm2);
		addVertex(uv0, vert5, norm2);
		EndPrimitive();

		// Back
		addVertex(uv2, vert6, norm5);
		addVertex(uv1, vert2, norm5);
		addVertex(uv3, vert4, norm5);
		addVertex(uv0, vert0, norm5);
		EndPrimitive();

	}
	else
	{

		// Pass-thru
		gl_Position = gl_PositionIn[0];
		EmitVertex();
		EndPrimitive();

	}

}
