#version 430 core

layout (binding = 0) uniform sampler2D geometrySample;

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

out vec4 color;

void main()
{
	color = texture(geometrySample, textureCoordinates);
}

