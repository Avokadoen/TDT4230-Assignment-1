#pragma once

#include <string>
#include "mesh.h"

Mesh generateTextGeometryBuffer(const std::string text, const float characterHeightOverWidth, const float totalTextWidth);