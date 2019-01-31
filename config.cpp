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

#include "config.h"

/*********************************************************************/
Config::Config()
{
    isFullScreen = 0;
    monitorID = -1;
    windowed_W = 1280;
    windowed_H = 720;
    videoPath = "";
    expectedFPS = 60;
    shadersPath = "shaders";

    fboWidth = windowed_W;
    fboHeight = windowed_H;
    fobHFOV = 60.0f;
    fobWFOV = 40.0f;
}

/*********************************************************************/
inline string trim(string& str)
{
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ')+1);
    return str;
}

/*********************************************************************/
bool Config::load(const char* configFilePath)
{
    string cfgContent = rx_read_file(configFilePath);
    if(cfgContent.size() < 2)
    {
        return false;
    }

    stringstream ss(cfgContent);
    string item;
    while (getline(ss, item, '\n'))
    {
        string line = trim(item);
        if(line.length() < 3)
        {
            continue;
        }

        if(line[0] == '#')
        {
            continue;
        }

        int idx = line.find_first_of('=');
        if(idx <= 0)
        {
            continue;
        }

        string key = line.substr(0, idx);
        string val = line.substr(idx + 1, line.length());

        if(0 == key.compare("videoPath"))
        {
            videoPath = val;
        }
        else if(0 == key.compare("isFullScreen"))
        {
            isFullScreen = atoi(val.c_str());
        }
        else if(0 == key.compare("monitorID"))
        {
            monitorID = atoi(val.c_str());
        }
        else if(0 == key.compare("windowed_W"))
        {
            windowed_W = atoi(val.c_str());
        }
        else if(0 == key.compare("windowed_H"))
        {
            windowed_H = atoi(val.c_str());
        }
        else if(0 == key.compare("shadersPath"))
        {
            shadersPath = val;
        }
        else if(0 == key.compare("expectedFPS"))
        {
            expectedFPS = atof(val.c_str());
        }
        else
        {
            printf("Error: Unknown config option: %s\n", line.c_str());
        }
    }

    return true;
}
