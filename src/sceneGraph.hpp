#pragma once

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stack>
#include <vector>
#include <cstdio>
#include <stdbool.h>
#include <cstdlib> 
#include <ctime> 
#include <chrono>
#include <fstream>

enum SceneNodeType {
	EMPTY					= 0b000001,
	GEOMETRY				= 0b000010,
	GEOMETRY_NORMAL_MAPPED	= 0b000100,
	GEOMETRY_2D				= 0b001000,
	POINT_LIGHT				= 0b010000,
	SPOT_LIGHT				= 0b100000,
};

struct SceneNode {
	SceneNode(SceneNodeType type) {
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);

		referencePoint = glm::vec3(0, 0, 0);
		vertexArrayObjectID = -1;
		diffuseTextureID = 0;
		normalMapID = 0;
		VAOIndexCount = 0;

		nodeType = type;
	}

	// A list of all children that belong to this node.
	// For instance, in case of the scene graph of a human body shown in the assignment text, the "Upper Torso" node would contain the "Left Arm", "Right Arm", "Head" and "Lower Torso" nodes in its list of children.
	std::vector<SceneNode*> children;
	
	// The node's position and rotation relative to its parent
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
	glm::mat4 currentTransformationMatrix;
	// A transformation matrix used to transform the mesh normals
	glm::mat3 normalMatrix;

	// The location of the node's reference point
	glm::vec3 referencePoint;

	// The ID of the VAO containing the "appearance" of this SceneNode.
	int vertexArrayObjectID;
	unsigned int VAOIndexCount;

	// Node type is used to determine how to handle the contents of a node
	SceneNodeType nodeType;

	// Optional textures
	GLuint diffuseTextureID;
	GLuint normalMapID;
};

SceneNode* createSceneNode(SceneNodeType type);
void addChild(SceneNode* parent, SceneNode* child);
void printNode(SceneNode* node);
int totalChildren(SceneNode* parent);

// For more details, see SceneGraph.cpp.