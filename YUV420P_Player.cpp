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

#include "YUV420P_Player.h"

/*********************************************************************/
YUV420P_Player::YUV420P_Player(Config* config)
  :vid_w(0)
  ,vid_h(0)
  ,y_tex(0)
  ,u_tex(0)
  ,v_tex(0)
  ,vert(0)
  ,frag(0)
  ,prog(0)
  ,initDone(0)
  ,cfg(config)
{

}

/*********************************************************************/
static void checkGLError(string msg, int line)
{
    #if 0
    GLenum err;
    if((err = glGetError()) != GL_NO_ERROR)
    {
        printf("openGL ERROR: %x - %s (line=%d)\n" , err, msg.c_str(), line);
    }
    #endif
}

/********************************************************************************************/
bool YUV420P_Player::setup(int vidW, int vidH)
{
    vid_w = vidW;
    vid_h = vidH;

    makeSphere(60, 0.f, 0.f, 0.f, 99.0f);

    glGenTextures(1, &y_tex);
    glBindTexture(GL_TEXTURE_2D, y_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w, vid_h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &u_tex);
    glBindTexture(GL_TEXTURE_2D, u_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w/2, vid_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
    glGenTextures(1, &v_tex);
    glBindTexture(GL_TEXTURE_2D, v_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w/2, vid_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    string YUV420P_VS = rx_read_file(cfg->shadersPath + "/video_vertex_shader.glsl");
    string YUV420P_FS = rx_read_file(cfg->shadersPath + "/video_fragment_shader.glsl");
    vert = rx_create_shader(GL_VERTEX_SHADER, YUV420P_VS.c_str());
    frag = rx_create_shader(GL_FRAGMENT_SHADER, YUV420P_FS.c_str());
    checkGLError("", __LINE__);

    prog = rx_create_program(vert, frag, false);
    checkGLError("", __LINE__);

    glBindFragDataLocation(prog, 0, "outColor");

    glLinkProgram(prog);
    rx_print_shader_link_info(prog);
    checkGLError("glLinkProgram", __LINE__);


    uMVPMatrixLocation = glGetUniformLocation(prog, "uMVPMatrix");
    checkGLError("", __LINE__);


    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkGLError("", __LINE__);
  
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);
    checkGLError("", __LINE__);

    aPositionLocation = glGetAttribLocation(prog, "aPosition");
    glVertexAttribPointer(aPositionLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(aPositionLocation);
    checkGLError("", __LINE__);

    aTextureCoordLocation = glGetAttribLocation(prog, "aTextureCoord");
    glVertexAttribPointer(aTextureCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(aTextureCoordLocation);
    checkGLError("", __LINE__);


    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), &indices[0], GL_STATIC_DRAW);
    checkGLError("", __LINE__);


    glUseProgram(prog);

    glUniform1i(glGetUniformLocation(prog, "y_tex"), 0);
    glUniform1i(glGetUniformLocation(prog, "u_tex"), 1);
    glUniform1i(glGetUniformLocation(prog, "v_tex"), 2);
    checkGLError("", __LINE__);

    // TODO
/*
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);

    glDeleteVertexArrays(1, &vao);
    */


    // distortion program

    // Generate FRAME BUFFER OBJECT
    //generate fbo id
    glGenFramebuffers(1, &fboId);
    checkGLError("", __LINE__);
    glGenTextures(1, &fboTex);
    checkGLError("", __LINE__);
    glGenRenderbuffers(1, &renderBufferId);
    checkGLError("", __LINE__);

    //Bind Frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    checkGLError("", __LINE__);
    //Bind texture
    glBindTexture(GL_TEXTURE_2D, fboTex);
    //Define texture parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cfg->fboWidth, cfg->fboHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGLError("", __LINE__);

    //Bind render buffer and define buffer dimension
    glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, cfg->fboWidth, cfg->fboHeight);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA8, cfg->fboWidth, cfg->fboHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBufferId);
    checkGLError("", __LINE__);

    int fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
		printf("Could not create FBO: %d\n", fboStatus);
    }

    string DISTORT_VS = rx_read_file(cfg->shadersPath + "/dist_vertex_shader.glsl");
    string DISTORT_FS = rx_read_file(cfg->shadersPath + "/dist_fragment_shader.glsl");
    int vert1 = rx_create_shader(GL_VERTEX_SHADER, DISTORT_VS.c_str());
    int frag1 = rx_create_shader(GL_FRAGMENT_SHADER, DISTORT_FS.c_str());
    checkGLError("", __LINE__);

    distort_prog = rx_create_program(vert1, frag1, false);
    checkGLError("", __LINE__);

    glBindFragDataLocation(distort_prog, 0, "outColor");

    glLinkProgram(distort_prog);
    rx_print_shader_link_info(distort_prog);
    checkGLError("glLinkProgram", __LINE__);

    float mTriangleVerticesData[] = {
            // X, Y, Z, U, V
            -1.0f, -1.0f, 0, 0.f, 0.f,
            1.0f, -1.0f, 0, 1.f, 0.f,
            -1.0f,  1.0f, 0, 0.f, 1.f,
            1.0f,  1.0f, 0, 1.f, 1.f,
    };

    glGenVertexArrays(1, &vaoDist);
    glBindVertexArray(vaoDist);
    checkGLError("", __LINE__);

    glGenBuffers(1, &vboDist);
    glBindBuffer(GL_ARRAY_BUFFER, vboDist);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mTriangleVerticesData) * sizeof(GLfloat), mTriangleVerticesData, GL_STATIC_DRAW);
    checkGLError("", __LINE__);

    aPositionDistLocation = glGetAttribLocation(distort_prog, "aPosition");
    glVertexAttribPointer(aPositionDistLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(aPositionDistLocation);
    checkGLError("", __LINE__);

    aTextureCoordDistLocation = glGetAttribLocation(distort_prog, "aTextureCoord");
    glVertexAttribPointer(aTextureCoordDistLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(aTextureCoordDistLocation);
    checkGLError("", __LINE__);


    glUseProgram(distort_prog);
    checkGLError("", __LINE__);

    glUniform1i(glGetUniformLocation(distort_prog, "sTexture"), 15);
    checkGLError("", __LINE__);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DITHER);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    resize(viewport[2], viewport[3]);

    initDone = 1;

    return true;
}

/********************************************************************************************/
void YUV420P_Player::resize(int winW, int winH)
{
    if(initDone)
    {
        return;
    }

    win_w = winW;
    win_h = winH;
    
    viewMatrix.identity();

    projectionMatrix.identity();
    projectionMatrix.perspective(cfg->fobHFOV, (float) cfg->fboWidth/cfg->fboHeight , 0.001f, 100.0f);
    //projectionMatrix.print();
    
    modelMatrix.identity();
    modelMatrix.rotate(M_PI/2, vec3(1, 0, 0));
}

/********************************************************************************************/
void YUV420P_Player::updateCamera(float y, float p, float r)
{
    viewMatrix.identity();
    viewMatrix.rotateX(y);
    viewMatrix.rotateY(p);
    viewMatrix.rotateZ(r);
}

/********************************************************************************************/
void YUV420P_Player::draw()
{
    if(!initDone)
    {
        return;
    }

    // render to FOB
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glViewport(0, 0, cfg->fboWidth, cfg->fboHeight);
    checkGLError("", __LINE__);

    
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLError("", __LINE__);

    glUseProgram(prog);

    mat4 mvpMatrix = (projectionMatrix * viewMatrix) * modelMatrix;
    glUniformMatrix4fv(uMVPMatrixLocation, 1, GL_FALSE, mvpMatrix.ptr());
    checkGLError("", __LINE__);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //checkGLError("", __LINE__);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, y_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, u_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, v_tex);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);

    // end of render to FOB
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError("", __LINE__);

    // render to display
    glViewport(0, 0, win_w, win_h);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(distort_prog);

    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_2D, fboTex);

    glBindVertexArray(vaoDist);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboDist);
    checkGLError("", __LINE__);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGLError("", __LINE__);
}

/********************************************************************************************/
void YUV420P_Player::setPixels(uint8_t* pixels[3], int stride[3])
{
    // TODO optimize the texture crop ... we have the last ypr
    glBindTexture(GL_TEXTURE_2D, y_tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w, vid_h, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

    glBindTexture(GL_TEXTURE_2D, u_tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w/2, vid_h/2, GL_RED, GL_UNSIGNED_BYTE, pixels[1]);

    glBindTexture(GL_TEXTURE_2D, v_tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride[2]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w/2, vid_h/2, GL_RED, GL_UNSIGNED_BYTE, pixels[2]);
}

/********************************************************************************************/
void YUV420P_Player::makeSphere(int nSlices, float x, float y, float z, float r)
{
    int max = nSlices + 1;
    int nVertices = max * max;
    float angleStepI = ((float) M_PI / nSlices);
    float angleStepJ = ((2.0f * (float) M_PI) / nSlices);

    for (int i = 0; i < max; i++)
    {
        for (int j = 0; j < max; j++)
        {
            float sini = (float) sin(((float) M_PI / nSlices) * i);
            float sinj = (float) sin(((2.0f * (float) M_PI) / nSlices) * j);
            float cosi = (float) cos(((float) M_PI / nSlices) * i);
            float cosj = (float) cos(((2.0f * (float) M_PI) / nSlices) * j);
            // vertex x,y,z
            vertices.push_back(x + r * sini * sinj);
            vertices.push_back(y + r * sini * cosj);
            vertices.push_back(z + r * cosi);
            // texture s,t
            vertices.push_back((float) j / (float) nSlices);
            vertices.push_back((1.0f - i) / (float)nSlices);
        }
    }

    for (int i = 0; i < nSlices; i++)
    {
        for (int j = 0; j < nSlices; j++)
        {
            int i1 = i + 1;
            int j1 = j + 1;
            indices.push_back((short) (i * max + j));
            indices.push_back((short) (i1 * max + j));
            indices.push_back((short) (i1 * max + j1));
            indices.push_back((short) (i * max + j));
            indices.push_back((short) (i1 * max + j1));
            indices.push_back((short) (i * max + j1));
        }
    }
}
