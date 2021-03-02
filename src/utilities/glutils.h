#pragma once

#include "mesh.h" // Mesh
#include "imageLoader.hpp" // PNGImage
#include "glad/glad.h"

struct GLIds {
	GLuint vao;
	GLuint vertex;
	GLuint normal;
	GLuint texture;
	GLuint index;
};

GLIds generateBuffer(const Mesh &mesh, bool dynamicTexture);

template <class T> void updateBuffer(GLuint vao, GLuint bufferID, const std::vector<T>& data) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, bufferID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(T), data.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

GLuint generateTexture(const PNGImage &pngImage);