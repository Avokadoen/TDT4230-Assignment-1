#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#include <iostream>

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

struct Tangents {
	glm::vec3 tangent;
	glm::vec3 bitTangent;
};

Tangents computeTangents(const glm::vec3& deltaPos1, const glm::vec3& deltaPos2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2) {
	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * deltaPos1.x - deltaUV1.y * deltaPos2.x);
	tangent.y = f * (deltaUV2.y * deltaPos1.y - deltaUV1.y * deltaPos2.y);
	tangent.z = f * (deltaUV2.y * deltaPos1.z - deltaUV1.y * deltaPos2.z);
	glm::vec3 bitTangent;
	bitTangent.x = f * (-deltaUV2.x * deltaPos1.x + deltaUV1.x * deltaPos2.x);
	bitTangent.y = f * (-deltaUV2.x * deltaPos1.y + deltaUV1.x * deltaPos2.y);
	bitTangent.z = f * (-deltaUV2.x * deltaPos1.z + deltaUV1.x * deltaPos2.z);

	return Tangents{
		tangent,
		bitTangent
	};
}

// GEOMETRY_NORMAL_MAPPED nodes should call this before being rendered
void appendTBNBuffer(Mesh &mesh, GLIds* ids) {
	unsigned int vertSize = mesh.vertices.size();

	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitTangents;
	tangents.reserve(vertSize);
	bitTangents.reserve(vertSize);

	for (int i = 0; i + 2 < mesh.vertices.size(); i += 3) {
		glm::vec3& v0 = mesh.vertices[i];
		glm::vec3& v1 = mesh.vertices[i + 1];
		glm::vec3& v2 = mesh.vertices[i + 2];

		glm::vec2& uv0 = mesh.textureCoordinates[i];
		glm::vec2& uv1 = mesh.textureCoordinates[i + 1];
		glm::vec2& uv2 = mesh.textureCoordinates[i + 2];

		Tangents tangents1 = computeTangents(v1 - v0, v2 - v0, uv1 - uv0, uv2 - uv0);
		Tangents tangents2 = computeTangents(v2 - v1, v0 - v1, uv2 - uv1, uv0 - uv1);
		Tangents tangents3 = computeTangents(v1 - v2, v0 - v2, uv1 - uv2, uv0 - uv2);

		tangents.push_back(tangents1.tangent);
		tangents.push_back(tangents2.tangent);
		tangents.push_back(tangents3.tangent);

		bitTangents.push_back(tangents1.bitTangent);
		bitTangents.push_back(tangents2.bitTangent);
		bitTangents.push_back(tangents3.bitTangent);
	}

	glBindVertexArray(ids->vao);

	generateAttribute(3, 3, tangents, false, false);
	generateAttribute(4, 3, bitTangents, false, false);

	glBindVertexArray(0);
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
