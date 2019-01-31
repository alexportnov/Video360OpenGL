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


#ifndef H264_DECODER_H
#define H264_DECODER_H

#define H264_INBUF_SIZE (1024*1024)

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "tinylib.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

typedef void(*h264_decoder_callback)(AVFrame* frame, void* user);

class H264_Decoder
{
 public:
    H264_Decoder(h264_decoder_callback frameCallback, void* user);
    ~H264_Decoder();                               
    bool load(std::string filepath);
    bool readFrame();
    void stop();

    bool running;

 private:
    AVCodec* codec;
    AVCodecContext* codec_context;
    AVCodecParserContext* parser;
    AVFrame* picture;
    FILE* fp;
    int frame;
    h264_decoder_callback cb_frame;
    void* cb_user;

    uint8_t* buffer;
    uint32_t dataRead;
};

#endif
