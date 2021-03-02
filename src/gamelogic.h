#pragma once

#include <utilities/window.hpp>
#include "sceneGraph.hpp"

void updateNodeTransformations(SceneNode* node, const glm::mat4& transformationThusFar);
void initGame(GLFWwindow* window, const CommandLineOptions options);
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);