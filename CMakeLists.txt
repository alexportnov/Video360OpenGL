cmake_minimum_required(VERSION 2.6)
  
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

project(Video360OpenGL)
mark_as_advanced(BUILD_OGL43)

find_package(OpenGL REQUIRED)

include_directories(".")
find_package(FFmpeg REQUIRED)

add_subdirectory(glxw)

IF(WIN32)
	set(THREADS_USE_PTHREADS_WIN32 true)
ENDIF()
find_package(Threads REQUIRED)
include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
message("pthread include dir ${THREADS_PTHREADS_INCLUDE_DIR}")

include_directories(glm)
include_directories(${CMAKE_BINARY_DIR}/glxw/include)
 
IF(WIN32)
	include_directories("win_libs/glfw-3.2.1.bin.WIN32/include")
	set(GLFW_LIBRARIES "win_libs/glfw-3.2.1.bin.WIN32/lib-vc2012/glfw3")
ELSE()
	set(GLFW_LIBRARIES "glfw")
ENDIF()

set(CMAKE_CXX_FLAGS "-O3 -finline-small-functions -fpermissive")
SET(LIBRARIES glxw ${GLFW_LIBRARIES} ${GLXW_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

if (FFMPEG_FOUND)
  message("Found FFMPEG/LibAV libraries => ${FFMPEG_INCLUDE_DIRS}")
  include_directories(${FFMPEG_INCLUDE_DIRS})
else (FFMPEG_FOUND)
  message("Can't find libavcodec, libavformat or libavutil. Add them!")
endif (FFMPEG_FOUND)

add_executable (Video360OpenGL main.cpp H264_Decoder.cpp YUV420P_Player.cpp config.cpp)
target_link_libraries(Video360OpenGL ${LIBRARIES} ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

if(MSVC)
    set_property(TARGET Video360OpenGL APPEND PROPERTY LINK_FLAGS /SAFESEH:NO)
endif()
