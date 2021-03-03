#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 2) vec2 textureCoordinates_in;

// orthogonal projection
uniform layout(location = 3) mat4 MP;

out layout(location = 1) vec2 textureCoordinates_out;

void main()
{
    textureCoordinates_out = vec2(textureCoordinates_in.x , 1.0 - textureCoordinates_in.y);
    gl_Position = MP * vec4(position, 1.0f);
}
