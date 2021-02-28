#pragma once

#include <glad/glad.h>

// Indices to shader variables
enum GeomtryVariables {
	VIEW_PROJECTION,
	TRANSFORM,
	NORMAL_MATRIX,
	VIEW_POSITION,
	AMBIENT,
	// Point light specific
	PL_POSITION,
	PL_COLOR,
	PL_CONSTANT,
	PL_LINEAR,
	PL_QUADRATIC,
	BALL_POSITION,
	BALL_RADIUS,
	MAX_GEOMTRY_VARS, // !Always last entry!
};

// Accepts a geometry shader program id and an array that will be 
// populated with location for each geomtry shader uniform variable. 
// Use GeomtryVariables enum to organize index access
// Array must be atleast MAX_GEOMTRY_VARS in size
void initializeGeomtryVariables(const GLint program, GLint* vars);
