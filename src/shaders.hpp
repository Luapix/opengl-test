#pragma once

#include "glfw.hpp"

extern const char* blockVertexShaderSource;
extern const char* blockFragmentShaderSource;

extern const char* cursorVertexShaderSource;
extern const char* overlayVertexShaderSource;
extern const char* colorFragmentShaderSource;

extern const char* entityVertexShaderSource;
extern const char* entityFragmentShaderSource;

extern const char* textVertexShaderSource;
extern const char* textFragmentShaderSource;

GlId loadBlockProgram();
GlId loadCursorProgram();
GlId loadColorOverlayProgram();
GlId loadEntityProgram();
GlId loadTextProgram();
