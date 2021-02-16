#version 430 core
// TODO: find a better way of setting size
const int POINT_LIGHTS = 3;

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 position;

uniform vec3 pointPosition[POINT_LIGHTS];
// uniform vec3 pointColor[POINT_LIGHTS]; 
uniform vec3 ambience;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

void main()
{
	vec3 normNorm = normalize(normal);

	// TODO: see uniform above
	vec3 pointColor = vec3(1f);
	// TODO: texture?
	vec3 objectColor = vec3(0.5f, 0.5f, 0.5f);

	// accumulative value for illumination  
	vec3 illumination = ambience;
	for (int i = 0; i < POINT_LIGHTS; i++) {
		vec3 lightDir = normalize(pointPosition[i] - position); 
		// calculate cosine of the angle between normal and lightDir
		float diffuse = max(dot(normal, lightDir), 0.0); 
		illumination += diffuse * pointColor;
	}

    color = vec4(objectColor * illumination, 1.0);
}