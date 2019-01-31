/*
** Copyright (c) 2016 AlexP
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef YUV420P_PLAYER_H
#define YUV420P_PLAYER_H

#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#include <GLXW/glxw.h>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define ROXLU_USE_MATH
#define ROXLU_USE_OPENGL
#include "tinylib.h"

#include "config.h"

using namespace std;

class YUV420P_Player {

 public:
    YUV420P_Player(Config* cfg);
    bool setup(int w, int h);
    void setPixels(uint8_t* pixels[3], int stride[3]);
    void draw();
    void resize(int winW, int winH);

    void updateCamera(float y, float p, float r);
 private:
  void makeSphere(int nSlices, float x, float y, float z, float r);
  void perspectiveM(mat4& m, float fovy, float aspect, float zNear, float zFar);

public:
    int vid_w;
    int vid_h;
    int win_w;
    int win_h;

 private:
    GLuint y_tex;
    GLuint u_tex;
    GLuint v_tex;
    GLuint vert;
    GLuint frag;
    GLuint prog;

    int initDone;

    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;

    GLuint aPositionLocation;
    GLuint uMVPMatrixLocation;
    GLuint aTextureCoordLocation;

    std::vector<GLfloat> vertices;
    std::vector<GLushort> indices;
    GLuint vbo;
    GLuint vao;
    GLuint ebo;

    GLuint distort_prog;
    GLuint renderBufferId;
    GLuint fboId;
    GLuint fboTex;
    GLuint vaoDist;
    GLuint vboDist;
    GLuint aPositionDistLocation;
    GLuint aTextureCoordDistLocation;

    Config* cfg;
};
#endif
