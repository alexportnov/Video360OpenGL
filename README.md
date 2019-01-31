# Video360OpenGL
This is pure C++/OpenGL implementation for 360° video player (spherical videos).
This made to be efficient; fully capable of running 120fps from a 4K video source on a reasonable machine.

Decode the video, push frames to OpenGL texture (textured over a sphere), render to framebuffer, render to display (look around using mouse). 
There's a preparation for distorting and post-processing.

All the examples are given for Linux based system (tested on Ubuntu18.04), since the project is based on cmake you should have no problem generating VisualStudio project. You may need to edit CMakeLists.txt for dependencies tho ...

## HowTo

1. Install dependencies
```sh
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libglfw3-dev
sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
sudo apt-get install xorg-dev libglu1-mesa-dev libglfw-dev
```

2. Compile
```sh
cmake ./
make
```

3. Run
```sh
./Video360OpenGL config.txt
```
The configuration file contains all the options - edit it.

## Convert movies
The input movies should be in raw h264 format (sorry for that). Here's how you can convert those:
```sh
ffmpeg -i input.mp4 -c:v libx264 -b:v 12M -maxrate 12M -an -f rawvideo -vf scale=1920:-1 -tune fastdecode -tune zerolatency output.h264
```

## Thanks to
Parts of the code is based on http://www.roxlu.com/2014/039/decoding-h264-and-yuv420p-playback - cheers!

glxw, glfw, https://github.com/roxlu/tinylib, ffmpeg, pthreads
