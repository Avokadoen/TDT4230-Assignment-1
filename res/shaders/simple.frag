#version 430 core
// TODO: find a better way of setting size
const int POINT_LIGHTS = 3;

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

uniform vec3 pointPosition[POINT_LIGHTS];
uniform vec3 ambience;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

void main()
{
	vec3 normNorm = normalize(normal);
	vec3 v3Color = (0.5 * normNorm + 0.5) * ambience;
    color = vec4(v3Color, 1.0);
}