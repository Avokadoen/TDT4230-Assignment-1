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
#include "utilities/shaderVariables.hpp"
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
Gloom::Shader* geometryShader;
Gloom::Shader* geometry2DShader;
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
glm::mat4 pers_projection;
glm::mat4 orth_projection;
glm::mat4 vpMat;
glm::mat4 cameraTransform;

// TODO: maybe use std::array or something less hacky
// array of all geometry shader variable locations
GLint geometryVars[MAX_GEOMETRY_VARS];
GLint geometry2DVars[MAX_GEOMETRY2D_VARS];

// Text nodes
SceneNode* testTextNode;

void mouseCallback(GLFWwindow* window, const double x, const double y) {
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

void initGame(GLFWwindow* window, const CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

	PNGImage charmap = loadPNGFile("../res/textures/charmap.png");
	GLint charMapId = generateTexture(charmap);

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

	geometryShader = new Gloom::Shader();
	geometryShader->makeBasicShader("../res/shaders/geometry.vert", "../res/shaders/geometry.frag");
	initializeGeomtryVariables(geometryShader->get(), geometryVars);

	geometry2DShader = new Gloom::Shader();
	geometry2DShader->makeBasicShader("../res/shaders/geometry_2D.vert", "../res/shaders/geometry_2D.frag");
	initializeGeomtry2DVariables(geometry2DShader->get(), geometry2DVars);
	
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
    rootNode = createSceneNode(GEOMETRY);
    boxNode  = createSceneNode(GEOMETRY);
    padNode  = createSceneNode(GEOMETRY);
    ballNode = createSceneNode(GEOMETRY);

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);

    boxNode->vertexArrayObjectID = boxVAO;
    boxNode->VAOIndexCount = box.indices.size();

    padNode->vertexArrayObjectID = padVAO;
    padNode->VAOIndexCount = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount = sphere.indices.size();

	{
		for (int i = 0; i < POINT_LIGHTS; i++) {
			pointLights.nodes[i] = createSceneNode(POINT_LIGHT);
			pointLights.constant[i] = 1;

			// Add light 0 and 1 as child of the pad and the ball node
			switch (i) {
			case 0:
				pointLights.color[i] = glm::vec3(1, 0, 0);

				pointLights.linear[i] = 0.002;
				pointLights.quadratic[i] = 0.0002;

				// Move pad light to avoid direct collision with ball (which would make things wonky)
				pointLights.nodes[i]->position = glm::vec3(0, 2, 0);
				padNode->children.push_back(pointLights.nodes[i]);
				break;
			case 1:
				// offset the light so it can cast proper shadows 
				pointLights.nodes[i]->position = glm::vec3(1, 1, 1);
				pointLights.color[i] = glm::vec3(0, 1, 0);

				pointLights.linear[i] = 0.005;
				pointLights.quadratic[i] = 0.0005;
				ballNode->children.push_back(pointLights.nodes[i]);
				break;
			default:
				// TODO: Random noise so that adding more than three lights will result in more interesting output
				pointLights.color[i] = glm::vec3(0, 0, 1);
				pointLights.linear[i] = 0.01;
				pointLights.quadratic[i] = 0.001;
				pointLights.nodes[i]->position = glm::vec3(0, -10, -80);
				rootNode->children.push_back(pointLights.nodes[i]);
				break;
			}
		}
		geometryShader->activate();
		glUniform1fv(geometryVars[PL_CONSTANT], POINT_LIGHTS, pointLights.constant);
		glUniform1fv(geometryVars[PL_LINEAR], POINT_LIGHTS, pointLights.linear);
		glUniform1fv(geometryVars[PL_QUADRATIC], POINT_LIGHTS, pointLights.quadratic);
		glUniform1f(geometryVars[BALL_RADIUS], radius);
		geometryShader->deactivate();
	}

	{
		// Create test text node
		std::string testText = "Hello world!";
		Mesh textMesh = generateTextGeometryBuffer(testText, 39/29, testText.length() * 29);
		const unsigned int textVAO = generateBuffer(textMesh);
		testTextNode = createSceneNode(GEOMETRY_2D);
		rootNode->children.push_back(testTextNode);
		testTextNode->vertexArrayObjectID = textVAO;
		testTextNode->VAOIndexCount = textMesh.indices.size();
		testTextNode->position = glm::vec3(0, 0, 0);
		testTextNode->textureID = charMapId;
	}
	// currently the application can't change window size, so we only construct this in setup. 
	// glfw support listening to window resize so we could move this there if we ever support it
	pers_projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);
	orth_projection = glm::ortho(0.0f, float(windowWidth), 0.0f, float(windowHeight), -1.0f, 1.0f);
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

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    cameraTransform = 
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    vpMat = pers_projection * cameraTransform;

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

void updateNodeTransformations(SceneNode* node, const glm::mat4& transformationThusFar) {
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
void renderNode(SceneNode* node, SceneNodeType nodeType) {
	if (node->nodeType == nodeType) {
		switch (node->nodeType) {
		case GEOMETRY:
			if (node->vertexArrayObjectID == -1) break;
			glUniformMatrix4fv(geometryVars[TRANSFORM], 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
			glUniformMatrix3fv(geometryVars[NORMAL_MATRIX], 1, GL_FALSE, glm::value_ptr(node->normalMatrix));
			glBindVertexArray(node->vertexArrayObjectID);
			glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
			break;
		case GEOMETRY_2D:
			if (node->vertexArrayObjectID == -1) break;
			glm::mat4 mp = node->currentTransformationMatrix * orth_projection;
			glUniformMatrix4fv(geometry2DVars[MP], 1, GL_FALSE, glm::value_ptr(mp));
			glBindVertexArray(node->vertexArrayObjectID);
			// TODO: default to an error texture if ID is not set
			// currently only use one texture unit (0)
			glBindTextureUnit(0, node->textureID);
			glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
			glBindTextureUnit(0, 0); 
			break;
		case GEOMETRY_NORMAL_MAPPED:
		case POINT_LIGHT:
		case SPOT_LIGHT:
		default:
			break;
		}
	}

    for(SceneNode* child : node->children) {
        renderNode(child, nodeType);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

	{
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
		geometryShader->activate();
		// TODO: only do update of ambient if the value changes
		glUniform3fv(geometryVars[AMBIENT], 1, glm::value_ptr(ambient));
		glUniformMatrix4fv(geometryVars[VIEW_PROJECTION], 1, GL_FALSE, glm::value_ptr(vpMat));
		auto cameraPositionPtr = glm::value_ptr(glm::vec3(cameraTransform[3]));
		glUniform3fv(geometryVars[VIEW_POSITION], 1, cameraPositionPtr);
		// TODO: we only need to send the 2 first lights, 
		//		 consider having some logic to only update moving lights
		// Send all light positions to the GPU
		glUniform3fv(geometryVars[PL_POSITION], POINT_LIGHTS, glm::value_ptr(positions[0]));
		glUniform3fv(geometryVars[PL_COLOR], POINT_LIGHTS, glm::value_ptr(pointLights.color[0]));
		glUniform3fv(geometryVars[BALL_POSITION], 1, glm::value_ptr(ballNode->position));
		renderNode(rootNode, GEOMETRY);
		geometryShader->deactivate();
	}
	

	{
		geometry2DShader->activate();
		// TODO: doesn't really make sense to use the same scene for UI?
		renderNode(rootNode, GEOMETRY_2D);
		geometry2DShader->deactivate();
	}
}
