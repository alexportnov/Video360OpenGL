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

#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
    #include <execinfo.h>
    #include <unistd.h>
#endif
#include <signal.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>

#include <GLXW/glxw.h>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define ROXLU_USE_MATH
#define ROXLU_USE_OPENGL
#define ROXLU_IMPLEMENTATION
#include "tinylib.h"

#include "H264_Decoder.h"
#include "YUV420P_Player.h"
#include "config.h"

H264_Decoder* decoder_ptr = NULL;
YUV420P_Player* player_ptr = NULL;
bool playback_initialized = false;
double prev_x;
double prev_y;


Config cfg;

pthread_t dec_thread;
sem_t texture_dump_now;
sem_t texture_dump_ready;
sem_t texture_dump_go;
bool frameReady;
AVFrame* currFrame;

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);

void frame_callback(AVFrame* frame, void* user);
void initialize_playback(AVFrame* frame);

/********************************************************************************************/
void handler(int sig)
{
#ifndef _WIN32
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
#endif
}

/********************************************************************************************/
void *decoderThread(void *ptr)
{
    printf("decoderThread started\n");
    H264_Decoder* dec = (H264_Decoder*)ptr;
    uint64_t prev = rx_hrtime();
    uint64_t frame_delay = (uint64_t)((1000.0f/cfg.expectedFPS) * 1000ull * 1000ull);

    while(dec->running)
    {
        uint64_t start = rx_hrtime();
        dec->readFrame();
        uint64_t dt = (rx_hrtime() - start);

        if(frame_delay > dt)
        {
            //printf("sleep for =%f ms frame_delay=%f \n", (float)(frame_delay - dt), (float)frame_delay);
#ifndef _WIN32
            usleep((frame_delay - dt)/1000);
#else
			Sleep((frame_delay - dt)/1000000);
#endif
        }

        uint64_t now = rx_hrtime();
        //printf("dec cycle =%f ms\n", (float)(now - prev)/(1000.0 * 1000));
        prev = now;
    }

    return NULL;
}

/********************************************************************************************/
int main(int argc, char* argv[])
{
    signal(SIGSEGV, handler);

    if(argc < 2)
    {
      printf("Error: too few arguments\n");
      printf("Usage: %s CFG_FILE_PATH\n", argv[0]);
      return -1;
    }

    if(false == cfg.load(argv[1]))
    {
        printf("Error: loading configuration file\n");
        return -1;
        
    }

    glfwSetErrorCallback(error_callback);
  
    printf("Init GL\n");
    if(!glfwInit())
    {
        printf("Error: cannot setup glfw.\n");
        return -1;
    }
  
    //glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_REFRESH_RATE, 60);
    
    printf("Set monitors\n");
    int numMonitors;
    GLFWmonitor** allMonitors = glfwGetMonitors(&numMonitors);
    int selectedMonitor = cfg.monitorID >= 0 ? cfg.monitorID : numMonitors-1;
    const GLFWvidmode *mode = glfwGetVideoMode(allMonitors[selectedMonitor]);
    GLFWwindow* win;

    int w = cfg.windowed_W;
    int h = cfg.windowed_H;
    prev_x = -1;
    prev_y = -1;

    if(cfg.isFullScreen)
    {
        w = mode->width;
        h = mode->height;

        //glfwWindowHint(GLFW_DOUBLEBUFFER, mode->height > 1000 ? GL_TRUE : GL_FALSE);
        win = glfwCreateWindow(w, h, "main", allMonitors[selectedMonitor], NULL);
    }
    else
    {
        //glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
        win = glfwCreateWindow(w, h, "main", NULL, NULL);
    }

    if(!win)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(win, resize_callback);
    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
    glfwSetCursorPosCallback(win, cursor_callback);
    glfwSetMouseButtonCallback(win, button_callback);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
  
    if(glxwInit() != 0)
    {
        printf("Error: cannot initialize glxw.\n");
        return -1;
    }

    printf("Init sensors\n");

    sem_init(&texture_dump_now, 0, 0);
    sem_init(&texture_dump_ready, 0, 0);
    sem_init(&texture_dump_go, 0, 0);
    
    frameReady = false;
    currFrame = NULL;

    printf("Init decoder\n");
    H264_Decoder decoder(frame_callback, NULL);
    if(!decoder.load(std::string(cfg.videoPath.c_str())))
    {
        return -1;
    }

    YUV420P_Player player(&cfg);
    
    player_ptr = &player;
    decoder_ptr = &decoder;
    
    pthread_attr_t tattr;
    int ret;
    int newprio = 20;
    sched_param param;

    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam(&tattr, &param);
    param.sched_priority = newprio;
    ret = pthread_attr_setschedparam(&tattr, &param);

    if(pthread_create(&dec_thread, &tattr, decoderThread, decoder_ptr))
    {

      fprintf(stderr, "Error creating thread\n");
      return -1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    uint64_t prev = rx_hrtime();

    printf("Main loop\n");

    while(!glfwWindowShouldClose(win))
    {
        if(frameReady)
        {
            sem_post(&texture_dump_now);

            sem_wait(&texture_dump_ready);

            if(currFrame != NULL)
            {
                if(!playback_initialized)
                {
                    playback_initialized = true;
                    if(!player.setup(currFrame->width, currFrame->height))
                    {
                      printf("Cannot setup the yuv420 player.\n");
                      return -1;
                    }
                }

                player_ptr->setPixels(currFrame->data, currFrame->linesize);
            }

            currFrame = NULL;
            sem_post(&texture_dump_go);
            frameReady = false;
        }

        uint64_t now = rx_hrtime();
        //printf("draw cycle =%f ms\n", (float)((now - prev)/(1000.0 * 1000)));
        prev = now;

        player.draw();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    sem_post(&texture_dump_now);
    sem_post(&texture_dump_go);

    decoder_ptr->stop();
    if(pthread_join(dec_thread, NULL))
    {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }

    glfwTerminate();
  
    sem_destroy(&texture_dump_now);
    sem_destroy(&texture_dump_ready);
    sem_destroy(&texture_dump_go);
    
    return 0;
}
 

/********************************************************************************************/
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if(action != GLFW_PRESS)
    {
        return;
    }

    switch(key)
    {
        case GLFW_KEY_ESCAPE:
        {
            glfwSetWindowShouldClose(win, GL_TRUE);
            break;
        }
    };
}

/********************************************************************************************/
void frame_callback(AVFrame* frame, void* user)
{
    frameReady = true;

    sem_wait(&texture_dump_now);

    currFrame = frame;
    sem_post(&texture_dump_ready);

    sem_wait(&texture_dump_go);
}

/********************************************************************************************/
void error_callback(int err, const char* desc)
{
    printf("GLFW error: %s (%d)\n", desc, err);
}

/********************************************************************************************/
void resize_callback(GLFWwindow* window, int width, int height)
{ 
    if(!player_ptr)
    {
        return;
    }

    player_ptr->resize(width, height);
}

/********************************************************************************************/
void button_callback(GLFWwindow* win, int bt, int action, int mods)
{
    double x,y;
    if(action == GLFW_PRESS || action == GLFW_REPEAT)
    { 
        glfwGetCursorPos(win, &x, &y);
    }
}

/********************************************************************************************/
void cursor_callback(GLFWwindow* win, double x, double y)
{
    if(!player_ptr)
    {
        return;
    }

    if(prev_x<0)
    {
        prev_x = x;
        prev_y = y;
        return;
    }

    //printf("cursor x,y = %d,%d\n", (int)x, (int)y);

    double dx = x - prev_x;
    double dy = y - prev_y;
    float lon = (3 * dx * 360 / player_ptr->win_w);
    float lat = (3 * dy * 180 / player_ptr->win_h);

    player_ptr->updateCamera(DEG_TO_RAD * lat, DEG_TO_RAD * lon, 0);
}

/********************************************************************************************/
void char_callback(GLFWwindow* win, unsigned int key)
{ 

}