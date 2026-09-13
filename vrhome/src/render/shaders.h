#pragma once

#include <GLES2/gl2.h>

// GLSL sources for every pipeline in the shell
extern const char* const kSceneVS;
extern const char* const kSceneFS;
extern const char* const kWarpVS;
extern const char* const kWarpFS;
extern const char* const kFloatVS;
extern const char* const kFloatFS;
extern const char* const kShapeVS;
extern const char* const kShapeFS;
extern const char* const kTextVS;
extern const char* const kTextFS;

// compile one stage; 0 on failure (log line already emitted)
GLuint compileShader(GLenum type, const char* src);
// compile+link a program; 0 on failure
GLuint linkProg(const char* vs, const char* fs);
