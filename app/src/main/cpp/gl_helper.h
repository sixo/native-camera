// Copyright (c) 2018 Roman Sisik

#ifndef NATIVE_CAMERA_GL_HELPER_H
#define NATIVE_CAMERA_GL_HELPER_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace sixo {

GLuint createShader(const char *src, GLenum type);

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);

void ortho(float *mat4, float left, float right, float bottom, float top, float near, float far);

}
#endif //NATIVE_CAMERA_GL_HELPER_H
