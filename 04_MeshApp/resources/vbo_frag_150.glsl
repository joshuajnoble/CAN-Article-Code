#version 150

// Uniforms
uniform float alpha;
uniform vec3 eyePoint;
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform vec3 lightPosition;
uniform float shininess;
uniform bool transform;
uniform float uvmix;

// Input attributes
in vec4 normal;
in vec4 position;
in vec4 uv;

// Output attributes
out vec4 color;

// Kernel
void main(void)
{

	// Transform point to quad
	if (transform)
	{

		// Initialize color
		color = vec4(0.0, 0.0, 0.0, 0.0);

		// Normalized eye position
		vec3 eye = normalize(-eyePoint);

		// Calculate light and reflection positions
		vec3 light = normalize(lightPosition.xyz - position.xyz);   
		vec3 reflection = normalize(-reflect(light, normal.xyz));

		// Calculate ambient, diffuse, and specular values
		vec4 ambient = lightAmbient;
		vec4 diffuse = clamp(lightDiffuse * max(dot(normal.xyz, light), 0.0), 0.0, 1.0);     
		vec4 specular = clamp(lightSpecular * pow(max(dot(reflection, eye), 0.0), 0.3 * shininess), 0.0, 1.0); 
	
		// Set color from light
		color = ambient + diffuse + specular;

		// Mix with UV map
		color = mix(color, vec4(uv.s, uv.t, 1.0, 1.0), uvmix);

		// Set alpha
		color.a = color.a * alpha;

	}
	else
	{

		// Color point white
		color = vec4(1.0, 1.0, 1.0, 1.0);
	
	}

}