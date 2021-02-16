#version 430 core
// TODO: find a better way of setting size
const int POINT_LIGHTS = 3;

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 position;

uniform vec3 viewPosition;
uniform vec3 pointPosition[POINT_LIGHTS];
// uniform vec3 pointColor[POINT_LIGHTS]; 
uniform vec3 ambient;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

void main()
{
	vec3 normNorm = normalize(normal);

	// TODO: see uniform above
	vec3 pointColor = vec3(1f);
	// TODO: texture?
	vec3 objectColor = vec3(0.5, 0.5, 0.5);
	float specularIntensity = 0.3;

	// accumulative value for illumination  
	vec3 illumination;
	for (int i = 0; i < POINT_LIGHTS; i++) {
		vec3 lightDir = normalize(pointPosition[i] - position); 

		// calculate cosine of the angle between normal and lightDir
		float diff = max(dot(normal, lightDir), 0.0); 
		vec3 diffuse = diff * pointColor;
		
		vec3 reflectDir = reflect(-lightDir, normal);  
		vec3 viewDir = normalize(viewPosition - position);
		float spec = max(pow(dot(reflectDir, viewDir), 32), 0);
		vec3 specular = spec * pointColor * specularIntensity;

		illumination += (ambient + diffuse + specular) * objectColor;
	}

    color = vec4(objectColor * illumination, 1.0);
}