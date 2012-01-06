#version 150

// Uniforms
uniform float amp;
uniform float phase;
uniform sampler2D positions;
uniform float scale;
uniform float speed;
uniform float width;

// Input attributes
in vec4 texCoord;

// Kernel
void main(void)
{

	// Read position from color value
	vec3 position = texture2D(positions, texCoord.st).rgb;

	// Use uniforms to update position
	float wave = (sin((phase * speed) + position.x * width)) * (amp * scale);
	position = vec3(scale * position.x, scale * position.y + wave, scale * position.z - wave);

	// Render position to color attachment
	gl_FragData[0] = vec4(position.x, position.y, position.z, 1.0);

}
