#pragma once

#include "mesh.h" // Mesh
#include "imageLoader.hpp" // PNGImage

unsigned int generateBuffer(Mesh &mesh);

GLuint generateTexture(const PNGImage &pngImage);