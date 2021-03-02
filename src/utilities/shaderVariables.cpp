#include "shaderVariables.hpp"

void initializeGeomtryVariables(const GLint program, geometry_vars *vars) {
	vars[VIEW_PROJECTION] = glGetUniformLocation(program, "VP");
	vars[TRANSFORM] = glGetUniformLocation(program, "mTransform");
	vars[NORMAL_MATRIX] = glGetUniformLocation(program, "normalMatrix");
	vars[AMBIENT] = glGetUniformLocation(program, "ambient");
	vars[VIEW_POSITION] = glGetUniformLocation(program, "viewPosition");

	vars[PL_POSITION] = glGetUniformLocation(program, "pLights.position");
	vars[PL_COLOR] = glGetUniformLocation(program, "pLights.color");
	vars[PL_CONSTANT] = glGetUniformLocation(program, "pLights.constant");
	vars[PL_LINEAR] = glGetUniformLocation(program, "pLights.linear");
	vars[PL_QUADRATIC] = glGetUniformLocation(program, "pLights.quadratic");

	vars[BALL_POSITION] = glGetUniformLocation(program, "ball.position");
	vars[BALL_RADIUS] = glGetUniformLocation(program, "ball.radius");
}

void initializeGeomtry2DVariables(const GLint program, geometry_2d_vars *vars) {
	vars[MVP] = glGetUniformLocation(program, "MVP");
}
