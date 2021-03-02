#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>

template <class T>
unsigned int generateAttribute(GLuint id, int elementsPerEntry, std::vector<T> data, bool normalize, bool dynamic) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}

// TODO: use enum bitmask for dynamic configuration
GLIds generateBuffer(const Mesh &mesh, bool dynamicTexture) {
	GLIds ids; 

    glGenVertexArrays(1, &ids.vao);
    glBindVertexArray(ids.vao);

	ids.vertex = generateAttribute(0, 3, mesh.vertices, false, false);
	if (mesh.normals.size() > 0) {
		ids.normal = generateAttribute(1, 3, mesh.normals, true, false);
	}
    if (mesh.textureCoordinates.size() > 0) {
        ids.texture = generateAttribute(2, 2, mesh.textureCoordinates, false, dynamicTexture);
    }

    glGenBuffers(1, &ids.index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids.index);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    return ids;
}

GLuint generateTexture(const PNGImage &pngImage) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pngImage.width, pngImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pngImage.pixels.data());

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	glBindTexture(GL_TEXTURE_2D, 0);
	return id;
}
