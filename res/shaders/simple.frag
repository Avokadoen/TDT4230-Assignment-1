#version 430 core
// TODO: find a better way of setting size
const int POINT_LIGHTS = 3;

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

uniform vec3 pointPosition[POINT_LIGHTS];


out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

void main()
{
    color = vec4(0.5 * normal + 0.5, 1.0);
}