#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"
// TODO: separate rendering and game logic into their own file
// TODO: use std ptr (shared, unique ...) instead of raw pointers
// TODO: code smell: can't move this thing or timestamps.h won't compile ... 
enum KeyFrameAction {
    BOTTOM, TOP
};
#include <timestamps.h>

#define POINT_LIGHTS 3

const glm::mat4 identity = glm::mat4(1.0f);

// Global ambient for phong shading
glm::vec3 ambient = glm::vec3(0.05f, 0.05f, 0.05f);

// SOA point light struct, because it makes it easier to integrate with 
// the scene graph code and to send multiple member data (and less system calls)
struct PointLights {
	// TODOS:
	SceneNode* nodes[POINT_LIGHTS];
	glm::vec3 color[POINT_LIGHTS]; 

	// Attenuation 
	float constant[POINT_LIGHTS];
	float linear[POINT_LIGHTS];
	float quadratic[POINT_LIGHTS];
};
PointLights pointLights;

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;


double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted = false;
bool hasLost = false;
bool jumpedToNextFrame = false;
bool isPaused = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;

// TODO: HACK: just keep the projection mat as a variable here for now
//		 it should not be in the scene graph for many reasons, so not quite sure of a 
//	     good location to keep this.
// the camera projection and transformation (VP)
glm::mat4 vpMat;
glm::mat4 cameraTransform;

// TODO: same as above, where should this go? 
//	     add some sort of lookup logic in shader perhaps? 
GLint vpLocation = -1;
GLint mTransformLocation = -1; // Model transform
GLint normalMatrixLocation = -1; // Normal transform
GLint ambientLocation = -1;
GLint viewPositionLocation = -1;

// Point light location
GLint plPosLocation = -1;
GLint plColLocation = -1;
GLint plConLocation = -1;
GLint plLinLocation = -1;
GLint plQuaLocation = -1;

// ball location
GLint ballPosLocation = -1;
GLint ballRadLocation = -1;

void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

	GLint program = shader->get();
	vpLocation = glGetUniformLocation(program, "VP");
	mTransformLocation = glGetUniformLocation(program, "mTransform");
	normalMatrixLocation = glGetUniformLocation(program, "normalMatrix");
	ambientLocation = glGetUniformLocation(program, "ambient");
	viewPositionLocation = glGetUniformLocation(program, "viewPosition");

	plPosLocation = glGetUniformLocation(program, "pLights.position");
	plColLocation = glGetUniformLocation(program, "pLights.color");
	plConLocation = glGetUniformLocation(program, "pLights.constant");
	plLinLocation = glGetUniformLocation(program, "pLights.linear");
	plQuaLocation = glGetUniformLocation(program, "pLights.quadratic");

	ballPosLocation = glGetUniformLocation(program, "ball.position");
	ballRadLocation = glGetUniformLocation(program, "ball.radius");

	float radius = 1.0f;

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(radius, 40, 40);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);

    boxNode->vertexArrayObjectID = boxVAO;
    boxNode->VAOIndexCount = box.indices.size();

    padNode->vertexArrayObjectID = padVAO;
    padNode->VAOIndexCount = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount = sphere.indices.size();

	for (int i = 0; i < POINT_LIGHTS; i++) {
		pointLights.nodes[i] = createSceneNode();

		pointLights.nodes[i]->position = glm::vec3(0);
		pointLights.nodes[i]->scale = glm::vec3(4);
		pointLights.nodes[i]->nodeType = SceneNodeType::POINT_LIGHT;
		
		pointLights.constant[i] = 1;

		// Add light 0 and 1 as child of the pad and the ball node
		switch (i) {
		case 0:
			pointLights.color[i] = glm::vec3(1, 0.4, 0.4);

			pointLights.linear[i] = 0.002;
			pointLights.quadratic[i] = 0.0002;

			// move light above pad
			pointLights.nodes[i]->position.y += 5;
			padNode->children.push_back(pointLights.nodes[i]);
			break;
		case 1:
			// offset the light so it can cast proper shadows 
			pointLights.nodes[i]->position = glm::vec3(1, 1, 1);
			pointLights.color[i] = glm::vec3(0.4, 1, 0.4);

			pointLights.linear[i] = 0.005;
			pointLights.quadratic[i] = 0.0005;
			ballNode->children.push_back(pointLights.nodes[i]);
			break;
		default:
			// TODO: Random noise so that adding more than three lights will result in more interesting output
			pointLights.color[i] = glm::vec3(0.4, 0.4, 1);
			pointLights.linear[i] = 0.01;
			pointLights.quadratic[i] = 0.001;
			pointLights.nodes[i]->position = glm::vec3(0, -10, -80);
			rootNode->children.push_back(pointLights.nodes[i]);
			break;
		}
	}

	glUniform1fv(plConLocation, POINT_LIGHTS, pointLights.constant);
	glUniform1fv(plLinLocation, POINT_LIGHTS, pointLights.linear);
	glUniform1fv(plQuaLocation, POINT_LIGHTS, pointLights.quadratic);
	glUniform1f(ballRadLocation, radius);

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();
    
	const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }
    
    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    cameraTransform = 
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    vpMat = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = { 
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x), 
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2), 
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    updateNodeTransformations(rootNode, identity);
}

void updateNodeTransformations(SceneNode* node, const glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

	if (node->nodeType == GEOMETRY) {
		// Calculate the normal transformation
		glm::mat3 rotationAndScale = glm::mat3(node->currentTransformationMatrix);
		node->normalMatrix = glm::transpose(glm::inverse(rotationAndScale));
	}

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

// should be called from renderFrame or self, or make sure to set vpLocation to current VP
void renderNode(SceneNode* node) {
	glUniformMatrix4fv(mTransformLocation, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));

	switch (node->nodeType) {
	case GEOMETRY:
		if (node->vertexArrayObjectID != -1) {
			glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(node->normalMatrix));
			glBindVertexArray(node->vertexArrayObjectID);
			glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
		}
		break;
	case POINT_LIGHT: break;
	case SPOT_LIGHT: break;
	
	}

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

	// TODO: only do update of ambient if the value changes
	glUniform3fv(ambientLocation, 1, glm::value_ptr(ambient));
	glUniformMatrix4fv(vpLocation, 1, GL_FALSE, glm::value_ptr(vpMat));
	auto cameraPositionPtr = glm::value_ptr(glm::vec3(cameraTransform[3]));
	glUniform3fv(viewPositionLocation, 1, cameraPositionPtr);

	// We update lights every frame as they are usually changing each frame
	// TODO: currently a hack to unwrap the position from point lights. 
	//		 do something less hacky instead
	glm::vec3 positions[POINT_LIGHTS];
	
	for (int i = 0; i < POINT_LIGHTS; i++) {
		// extract world position from transform: make member [3][0], [3][1] and [3][2] to a vec3
		// these entries happens to be the current translation in the transform and should not be affected
		// by any other transformation
		positions[i] = glm::vec3(pointLights.nodes[i]->currentTransformationMatrix[3]);
	}
	// TODO: we only need to send the 2 first lights, 
	//		 consider having some logic to only update moving lights
	// Send all light positions to the GPU
	glUniform3fv(plPosLocation, POINT_LIGHTS, glm::value_ptr(positions[0]));
	glUniform3fv(plColLocation, POINT_LIGHTS, glm::value_ptr(pointLights.color[0]));
	glUniform3fv(ballPosLocation, 1, glm::value_ptr(ballNode->position));

    renderNode(rootNode);
}
