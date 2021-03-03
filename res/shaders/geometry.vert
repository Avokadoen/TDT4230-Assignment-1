#version 430 core
in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangent;
in layout(location = 4) vec3 bitTangent;

uniform layout(location = 0) mat4 VP;
uniform layout(location = 1) mat4 mTransform;
uniform layout(location = 2) mat3 normalMatrix; // Inverse transpose transform

out layout(location = 1) vec2 textureCoordinates_out;
out layout(location = 2) vec3 position_out;
out layout(location = 3) mat3 tbn_out;

void main()
{
	textureCoordinates_out = textureCoordinates_in;
	
	vec4 preProjPos = mTransform * vec4(position, 1.0f);
	position_out = vec3(preProjPos);

	vec3 t = normalize(normalMatrix * tangent);
	vec3 b = normalize(normalMatrix * bitTangent);
	vec3 n = normalize(normalMatrix * normal_in);
	tbn_out = mat3(t, b, n);

	gl_Position = VP * preProjPos;
}
