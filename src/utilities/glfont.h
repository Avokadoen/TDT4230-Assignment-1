#pragma once

#include <string>
#include "mesh.h"

struct TextMesh {
	Mesh mesh;
	std::string text;
	unsigned int maxLength;
};

TextMesh generateTextGeometryBuffer(const std::string text, const float characterHeightOverWidth, const float totalTextWidth);

// Update text on mesh. If text exceed initial text, then the remaining characters are omitted.
void updateTextGeometryBuffer(TextMesh& TextMesh, const std::string text);