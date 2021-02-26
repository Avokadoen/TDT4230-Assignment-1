#pragma once

#include "mesh.h" // Mesh
#include "imageLoader.hpp" // PNGImage

unsigned int generateBuffer(Mesh &mesh);


typedef GLuint TextureID;
TextureID generateTexture(const PNGImage &pngImage);