#version 430 core
// TODO: find a better way of setting size
const int POINT_LIGHTS = 3;

struct PointLights {
	vec3 position[POINT_LIGHTS];
	vec3 color[POINT_LIGHTS];
	
	// Attenuation
	float constant[POINT_LIGHTS];
	float linear[POINT_LIGHTS];
	float quadratic[POINT_LIGHTS]; 
};

// TODO: position here can be merged with light position for better cache
struct Ball {
	vec3 position;
	float radius;
};

in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 position;
in layout(location = 3) mat3 tbn;

layout (binding = 0) uniform sampler2D diffuseSample;
layout (binding = 1) uniform sampler2D normalSample;
layout (binding = 2) uniform sampler2D roughnessSample;

uniform vec3 viewPosition;
uniform PointLights pLights;
uniform vec3 ambient;
uniform int isNormalMapped; 


uniform Ball ball;
// TODO: light member, ball member, based on distance between light and ball? 
const float softRadius = 0.3f;

out vec4 color;

float rand(vec2 co) { 
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); 
}

float dither(vec2 uv) { 
	return (rand(uv)*2.0-1.0) / 256.0; 
}

vec3 reject(vec3 from, vec3 onto) {
	return from - onto * dot(from, onto) / dot(onto, onto);
}

void main()
{
	const float specularIntensity = 1;
	float shininess;  
	vec3 normal;
	vec4 objectColor; 
	if (isNormalMapped == 1) {
		normal = texture(normalSample, textureCoordinates).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = tbn * normal;
		// TODO: doesn't really make sense that: normal mapped == color mapped. Maybe rename to isTextureMapped
		objectColor = texture(diffuseSample, textureCoordinates);
		shininess = 5 / pow(texture(roughnessSample, textureCoordinates).r, 2);
	} else {
		normal = normalize(tbn[2]);
		objectColor = vec4(0.5, 0.5, 0.5, 1.0);
		shininess = 32;
	}


	// accumulative value for illumination  
	vec3 illumination = ambient;
	for (int i = 0; i < POINT_LIGHTS; i++) {
		// Calculate shadow for ball
		vec3 posLightVec = pLights.position[i] - position;
		vec3 posBall = position - ball.position;
		vec3 rejection = reject(posBall, posLightVec); 
		bool isLightBlocked = length(posLightVec) > length(posBall) && dot(posLightVec, posBall) <= 0;	
		float softShadowPos = min(max(ball.radius - length(rejection), 0), softRadius);
		float softShadow = 1 - (softShadowPos / softRadius);

		// if light is blocked, then we dont apply shadow
		float shadow = min(max(softShadow + float(!isLightBlocked), 0), 1);

		// Calculate the attenuation
		float posLightMagnitude = length(posLightVec);
		float attenuation = 1 / (pLights.constant[i] + pLights.linear[i] * posLightMagnitude + pLights.quadratic[i] * pow(posLightMagnitude, 2));
		
		vec3 lightDir = normalize(posLightVec); 
		// calculate cosine of the angle between normal and lightDir
		float diff = max(dot(normal, lightDir), 0.0); 
		vec3 diffuse = (diff * pLights.color[i]) * attenuation;
		
		vec3 reflectDir = reflect(-lightDir, normal);  
		vec3 viewDir = normalize(viewPosition - position);
		float spec = max(pow(dot(reflectDir, viewDir), shininess), 0);
		vec3 specular = (spec * pLights.color[i] * specularIntensity) * attenuation;

		illumination += (diffuse + specular) * objectColor.xyz * shadow;
	}

    color = vec4(objectColor.xyz * illumination + dither(textureCoordinates), objectColor.w);
}

