#include <iostream>
#include "glfont.h"

Mesh generateTextGeometryBuffer(const std::string text, const float characterHeightOverWidth, const float totalTextWidth) {
	// bitmap allocates 29 pixels for each character in width
	const int CHAR_PX_WIDTH = 29; 
	// bitmap total width  
	const int BITMAP_WIDTH = CHAR_PX_WIDTH * 128;

	float characterWidth = totalTextWidth / float(text.length());
	float characterHeight = characterHeightOverWidth * characterWidth;

	unsigned int vertexCount = 4 * text.length();
	unsigned int indexCount = 6 * text.length();

	Mesh mesh;

	mesh.vertices.resize(vertexCount);
	mesh.textureCoordinates.resize(vertexCount);
	mesh.indices.resize(indexCount);

	for(unsigned int i = 0; i < text.length(); i++)
	{
		float baseXCoordinate = float(i) * characterWidth;

		mesh.vertices.at(4 * i + 0) = { baseXCoordinate, 0, 0 };
		mesh.vertices.at(4 * i + 1) = { baseXCoordinate + characterWidth, 0, 0 };
		mesh.vertices.at(4 * i + 2) = { baseXCoordinate + characterWidth, characterHeight, 0 };
		mesh.vertices.at(4 * i + 3) = { baseXCoordinate, characterHeight, 0 };

		char c = text[i];
		const float BASE_PX_X = c * CHAR_PX_WIDTH;
		const float SMALL_U = BASE_PX_X / BITMAP_WIDTH;
		const float BIG_U = (BASE_PX_X + CHAR_PX_WIDTH) / BITMAP_WIDTH;
		mesh.textureCoordinates.at(4 * i + 0) = { SMALL_U, 0 };
		mesh.textureCoordinates.at(4 * i + 1) = { BIG_U, 0 };
		mesh.textureCoordinates.at(4 * i + 2) = { BIG_U, 1 };
		mesh.textureCoordinates.at(4 * i + 3) = { SMALL_U, 1 };

		mesh.indices.at(6 * i + 0) = 4 * i + 0;
		mesh.indices.at(6 * i + 1) = 4 * i + 1;
		mesh.indices.at(6 * i + 2) = 4 * i + 2;
		mesh.indices.at(6 * i + 3) = 4 * i + 0;
		mesh.indices.at(6 * i + 4) = 4 * i + 2;
		mesh.indices.at(6 * i + 5) = 4 * i + 3;
	}

	return mesh;
}