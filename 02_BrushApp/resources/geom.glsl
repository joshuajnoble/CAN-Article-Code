#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable 

// Uniforms
uniform float aspect;
uniform float size;
uniform bool transform;

// Output attributes
varying vec2 uv;

// Kernel
void main(void)
{

	// Check the transform flag
	if (transform)
	{

		// To draw a square, we will draw two triangles, just like 
		// above. The two triangles will share vertices, so we'll define
		// the vertices in advance.
		vec4 vert0 = vec4(cos(radians(315.0)) * size, sin(radians(315.0)) * size * aspect, 0.0, 0.0);
		vec4 vert1 = vec4(cos(radians(225.0)) * size, sin(radians(225.0)) * size * aspect, 0.0, 0.0);
		vec4 vert2 = vec4(cos(radians(135.0)) * size, sin(radians(135.0)) * size * aspect, 0.0, 0.0);
		vec4 vert3 = vec4(cos(radians(45.0)) * size, sin(radians(45.0)) * size * aspect, 0.0, 0.0);

		// Because we are going to texture our quad, we'll need to define
		// UV coordinates, as well. Whenever you call EmitVertex(), it passes
		// the current state to the fragment shader. By setting "uv" before
		// each EmitVertex() call, we can pass customer properties.
		vec2 uv0 = vec2(0.0, 0.0);
		vec2 uv1 = vec2(0.0, 1.0);
		vec2 uv2 = vec2(1.0, 1.0);
		vec2 uv3 = vec2(1.0, 0.0);

		// Draw the left triangle
		uv = uv0;
		gl_Position = gl_PositionIn[0] + vert0;
		EmitVertex();
		uv = uv1;
		gl_Position = gl_PositionIn[0] + vert1;
		EmitVertex();
		uv = uv3;
		gl_Position = gl_PositionIn[0] + vert3;
		EmitVertex();

		// Close this triangle
		EndPrimitive();
				
		// And now the right one
		uv = uv3;
		gl_Position = gl_PositionIn[0] + vert3;
		EmitVertex();
		uv = uv1;
		gl_Position = gl_PositionIn[0] + vert1;
		EmitVertex();
		uv = uv2;
		gl_Position = gl_PositionIn[0] + vert2;
		EmitVertex();

		// Close the second triangle to form a quad
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
