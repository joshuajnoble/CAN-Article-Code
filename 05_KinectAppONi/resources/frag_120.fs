#version 120

// Uniforms
uniform vec3 eyePoint;
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform vec3 lightPosition;
uniform float shininess;
uniform bool transform;
uniform float uvmix;

// Input attributes;
varying vec4 normalOut;
varying vec4 positionOut;
varying vec4 uvOut;
varying float brightnessOut;

// Kernel
void main(void)
{

	// Initialize color
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);

	// Transform point to box
	if (transform)
	{

		// Normalized eye position
		vec3 eye = normalize(-eyePoint);

		// Calculate light and reflection positions
		vec3 light = normalize(lightPosition.xyz - positionOut.xyz);   
		vec3 reflection = normalize(-reflect(light, normalOut.xyz));

		// Calculate ambient, diffuse, and specular values
		vec4 ambient = lightAmbient;
		vec4 diffuse = clamp(lightDiffuse * max(dot(normalOut.xyz, light), 0.0), 0.0, 1.0);     
		vec4 specular = clamp(lightSpecular * pow(max(dot(reflection, eye), 0.0), 0.3 * shininess), 0.0, 1.0); 
	
		// Set color from light
		color = lightAmbient + lightDiffuse + lightSpecular;

		// Mix with UV map
		color = mix(color, vec4(uvOut.s, uvOut.t, 1.0, 1.0), uvOut);

	}
	else
	{

		// Color point white
		color = vec4(1.0, 1.0, 1.0, brightnessOut > 0.0 ? 1.0 : 0.0);
	
	}

	// Set final color
	gl_FragColor = color;

}
