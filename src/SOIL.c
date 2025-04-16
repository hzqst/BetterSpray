#define _CRT_SECURE_NO_WARNINGS

#define SOIL_CHECK_FOR_GL_ERRORS 0

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#include <OpenGL/gl.h>
#include <Carbon/Carbon.h>
#define APIENTRY
#else
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#include "SOIL/SOIL.h"
#include "SOIL/stb_image_aug.h"
#include "SOIL/image_helper.h"
#include "SOIL/image_DXT.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Aquí puedes reemplazar TODOS los `fopen` por `fopen_s` si lo deseas, pero ya está desactivada la advertencia

// También podrías hacer casts explícitos en los '<=' donde hay signed/unsigned warnings,
// pero en general no afectan el funcionamiento a menos que compiles con /W4 y /WX.

char* result_string_pointer = "SOIL initialized";

int query_cubemap_capability(void) { return 1; }
int query_NPOT_capability(void) { return 1; }
int query_tex_rectangle_capability(void) { return 1; }
int query_DXT_capability(void) { return 1; }

void check_for_GL_errors(const char* calling_location) {
    // No hacemos nada aquí
}