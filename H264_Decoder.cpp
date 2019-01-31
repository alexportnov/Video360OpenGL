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


#include "H264_Decoder.h"

H264_Decoder::H264_Decoder(h264_decoder_callback frameCallback, void* user) 
  :codec(NULL)
  ,codec_context(NULL)
  ,parser(NULL)
  ,fp(NULL)
  ,frame(0)
  ,cb_frame(frameCallback)
  ,cb_user(user)
{
    avcodec_register_all();

    buffer = new uint8_t[2 * H264_INBUF_SIZE + 64];
    dataRead = 0;
}

/********************************************************************************************/
H264_Decoder::~H264_Decoder()
{
    if(parser)
    {
        av_parser_close(parser);
        parser = NULL;
    }

    if(codec_context)
    {
        avcodec_close(codec_context);
        av_free(codec_context);
        codec_context = NULL;
    }

    if(picture)
    {
        av_free(picture);
        picture = NULL;
    }

    if(fp)
    {
        fclose(fp);
        fp = NULL;
    }

    delete[] buffer;

    cb_frame = NULL;
    cb_user = NULL;
    frame = 0;
}

/********************************************************************************************/
bool H264_Decoder::load(std::string filepath)
{
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec)
    {
        printf("Error: cannot find the h264 codec: %s\n", filepath.c_str());
        return false;
    }

    codec_context = avcodec_alloc_context3(codec);

    if(avcodec_open2(codec_context, codec, NULL) < 0)
    {
        printf("Error: could not open codec.\n");
        return false;
    }

    fp = fopen(filepath.c_str(), "rb");
    if(!fp)
    {
        printf("Error: cannot open: %s\n", filepath.c_str());
        return false;
    }

    picture = av_frame_alloc();
    if(!picture)
    {
        printf("Erorr: av_frame_alloc\n");
        return false;
    }

    parser = av_parser_init(codec->id);
    if(!parser)
    {
        printf("Erorr: cannot create H264 parser.\n");
        return false;
    }

    running = true;

    return true;
}

/********************************************************************************************/
void H264_Decoder::stop()
{
    running = false;
}

/********************************************************************************************/
bool H264_Decoder::readFrame()
{
    if(dataRead < H264_INBUF_SIZE/2)
    {
        int bytes_read = (int)fread(buffer + dataRead, 1, H264_INBUF_SIZE, fp);
        if(bytes_read <= 0)
        {
          printf("H264_Decoder::readFrame NO MORE DATA\n");
          rewind(fp);
          return false;
        }
        dataRead += bytes_read;
    }

    if(dataRead == 0)
    {
      printf("H264_Decoder::readFrame NO MORE DATA\n");
      return false;
    }

    //printf("H264_Decoder::update dataRead=%d\n", dataRead);

    uint8_t *in_data = buffer;
    int in_data_size = dataRead;
    uint8_t* data = NULL;
    int size = 0;
    AVPacket pkt;
    av_init_packet(&pkt);

    while (in_data_size > 0)
    {
        int len = av_parser_parse2(parser, codec_context, &data, &size, in_data, in_data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (len < 0)
        {
            fprintf(stderr, "Error while parsing\n");
            return false;
        }

        //printf("H264_Decoder::update len=%d size=%d\n", len, size);
        in_data += len;
        in_data_size -= len;

        if(size)
        {
            int got_picture = 0;
            int used = 0;

            //uint64_t now = rx_hrtime();
            pkt.data = data;
            pkt.size = size;

            //printf("H264_Decoder::decodeFrame size=%d\n", size);
            used = avcodec_decode_video2(codec_context, picture, &got_picture, &pkt);
            if(used < 0)
            {
                printf("Error while decoding a frame.\n\n");
            }

            //printf("len=%d got_picture=%d\n", len, got_picture);
            if(got_picture)
            {
                ++frame;
                cb_frame(picture, cb_user);
            }

            //printf("dec=%f ms\n", (float)((rx_hrtime() - now)/(1000.0 * 1000)));

            // can be optimized
            memcpy(buffer, buffer + used, dataRead - size);
            dataRead -= used;

            //printf("H264_Decoder::update dataRead=%d\n", dataRead);
            break;
        }
    }

    return true;
}
