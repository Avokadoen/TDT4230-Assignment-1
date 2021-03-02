#pragma once

#include <glad/glad.h>

// TODO: this might be too shallow abstraction
//	     find an abstraction preferably that can be embeded in the Gloom:shader type 
//		 and is not a hashmap

// TODO: strongly type vars in each function for type safety
// see: https://stackoverflow.com/a/32848342/11768869

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
	MAX_GEOMETRY_VARS, // !Always last entry!
};

// Accepts a geometry shader program id and an array that will be 
// populated with location for each geomtry shader uniform variable. 
// Use GeomtryVariables enum to organize index access
// Array must be atleast MAX_GEOMETRY_VARS in size
void initializeGeomtryVariables(const GLint program, GLint* vars) {
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

enum Geometry2DVariables {
	MP, // Model View Projection
	MAX_GEOMETRY2D_VARS, // !Always last entry!
};

// Accepts a geometry shader program id and an array that will be 
// populated with location for each geomtry shader uniform variable. 
// Use Geometry2DVariables enum to organize index access
// Array must be atleast MAX_GEOMETRY2D_VARS in size
void initializeGeomtry2DVariables(const GLint program, GLint* vars) {
	vars[MP] = glGetUniformLocation(program, "MP");
}