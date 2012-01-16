#version 120
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable 

// Constants to match those in CPP
const int SHAPE_CIRCLE = 2;
const int SHAPE_SQUARE = 1;
const int SHAPE_TRIANGLE = 0;

// Uniforms
uniform float aspect;
uniform int shape;
uniform float size;
uniform bool transform;

// Kernel
void main(void)
{

	// Check the transform flag
	if (transform)
	{

		// Note that we use the window's aspect ratio
		// on all Y values. This is because we're doing 2D 
		// drawing in a 3D space. Once you move into pure 3D. 
		// you'll probably not want to apply aspect all over.

		// Key on shape
		if (shape == SHAPE_TRIANGLE)
		{


			// To draw a triangle, we add the size times the cos 
			// and sin of each angle to the original position.
			// After each point is set, we call EmitVertex() to
			// add a vertex to the geometry. Be sure to draw 
			// counter-clockwise.
			gl_Position = gl_PositionIn[0] + vec4(cos(radians(270.0)) * size, sin(radians(270.0)) * size * aspect, 0.0, 0.0);
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vec4(cos(radians(150.0)) * size, sin(radians(150.0)) * size * aspect, 0.0, 0.0);
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vec4(cos(radians(30.0)) * size, sin(radians(30.0)) * size * aspect, 0.0, 0.0);
			EmitVertex();
			
			// Call EndPrimitive() to close the shape. Note that this 
			// is not needed if you are only drawing one primitive, since 
			// it is implied
			EndPrimitive();

		} else if (shape == SHAPE_SQUARE) {

			// To draw a square, we will draw two triangles, just like 
			// above. The two triangles will share vertices, so we'll define
			// the vertices in advance.
			vec4 vert0 = vec4(cos(radians(315.0)) * size, sin(radians(315.0)) * size * aspect, 0.0, 0.0);
			vec4 vert1 = vec4(cos(radians(225.0)) * size, sin(radians(225.0)) * size * aspect, 0.0, 0.0);
			vec4 vert2 = vec4(cos(radians(135.0)) * size, sin(radians(135.0)) * size * aspect, 0.0, 0.0);
			vec4 vert3 = vec4(cos(radians(45.0)) * size, sin(radians(45.0)) * size * aspect, 0.0, 0.0);

			// Draw the left triangle
			gl_Position = gl_PositionIn[0] + vert0;
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vert1;
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vert3;
			EmitVertex();

			// Close this triangle
			EndPrimitive();
			
			// And now the right one
			gl_Position = gl_PositionIn[0] + vert3;
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vert1;
			EmitVertex();
			gl_Position = gl_PositionIn[0] + vert2;
			EmitVertex();

			// Close the second triangle to form a quad
			EndPrimitive();

		} else if( shape == SHAPE_CIRCLE) {

			// To draw a circle, we'll iterate through 24
			// segments and form triangles
			float twoPi = 6.283185306;
			float delta = twoPi / 24.0;
			for (float theta = 0.0; theta < twoPi; theta += delta)
			{

				// Draw a triangle to form a wedge of the circle
				gl_Position = gl_PositionIn[0] + vec4(cos(theta) * size, sin(theta) * size * aspect, 0.0, 0.0);
				EmitVertex();
				gl_Position = gl_PositionIn[0];
				EmitVertex();
				gl_Position = gl_PositionIn[0] + vec4(cos(theta + delta) * size, sin(theta + delta) * size * aspect, 0.0, 0.0);
				EmitVertex();
				EndPrimitive();

			}
			
		}

	}
	else
	{

		// Pass-thru
		gl_Position = gl_PositionIn[0];
		EmitVertex();
		EndPrimitive();

	}

}
