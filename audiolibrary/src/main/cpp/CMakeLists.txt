
cmake_minimum_required(VERSION 3.4.1)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/jni)
include_directories(${CMAKE_CURRENT_LIST_DIR}/decode)
include_directories(${CMAKE_CURRENT_LIST_DIR}/message)
include_directories(${CMAKE_CURRENT_LIST_DIR}/error)
include_directories(${CMAKE_CURRENT_LIST_DIR}/opensl)
include_directories(${CMAKE_CURRENT_LIST_DIR}/queue)
include_directories(${CMAKE_CURRENT_LIST_DIR}/sonic)

# soundtouch 头文件
set(SOUND_TOUCH ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/soundtouch)
include_directories(${SOUND_TOUCH}/include)
include_directories(${SOUND_TOUCH}/source)

# ffmpeg 库设置
set(FFMPEG_LIB ${CMAKE_CURRENT_LIST_DIR}/../../../../extra/ffmpeg/${ANDROID_ABI})
set(FFMPEG_HEADER ${FFMPEG_LIB}/include)
include_directories(${FFMPEG_HEADER})

add_library(ltpffmpeg SHARED IMPORTED)
set_target_properties(ltpffmpeg PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB}/libltpffmpeg.so)

file(GLOB soundtouch_source
        ${SOUND_TOUCH}/source/AAFilter.cpp
        ${SOUND_TOUCH}/source/BPMDetect.cpp
        ${SOUND_TOUCH}/source/cpu_detect_x86.cpp
        ${SOUND_TOUCH}/source/FIFOSampleBuffer.cpp
        ${SOUND_TOUCH}/source/FIRFilter.cpp
        ${SOUND_TOUCH}/source/InterpolateCubic.cpp
        ${SOUND_TOUCH}/source/InterpolateLinear.cpp
        ${SOUND_TOUCH}/source/InterpolateShannon.cpp
        ${SOUND_TOUCH}/source/mmx_optimized.cpp
        ${SOUND_TOUCH}/source/PeakFinder.cpp
        ${SOUND_TOUCH}/source/RateTransposer.cpp
        ${SOUND_TOUCH}/source/SoundTouch.cpp
        ${SOUND_TOUCH}/source/sse_optimized.cpp
        ${SOUND_TOUCH}/source/TDStretch.cpp
        )

file(GLOB audioplayer_source
        ${CMAKE_CURRENT_LIST_DIR}/jni/audio_jni.cc
        ${CMAKE_CURRENT_LIST_DIR}/jni/audio_env.c
        ${CMAKE_CURRENT_LIST_DIR}/audio_player.cc
        ${CMAKE_CURRENT_LIST_DIR}/message/handler.cc
        ${CMAKE_CURRENT_LIST_DIR}/message/message_queue.cc
        ${CMAKE_CURRENT_LIST_DIR}/decode/audio_decoder.cc
        ${CMAKE_CURRENT_LIST_DIR}/error/error.cc
        ${CMAKE_CURRENT_LIST_DIR}/opensl/audio_renderer.cc
        ${CMAKE_CURRENT_LIST_DIR}/queue/buffer_queue.cc
        ${CMAKE_CURRENT_LIST_DIR}/sonic/sonic.c
        )

add_library(
        ltpaudio
        SHARED
        ${audioplayer_source}
        ${soundtouch_source}
)

target_link_libraries(
        ltpaudio
        android
        OpenSLES
        log
        z
        ltpffmpeg
)

#target_compile_options(ltpaudio PUBLIC -fsanitize=address -fno-omit-frame-pointer)
#set_target_properties(ltpaudio PROPERTIES LINK_FLAGS -fsanitize=address)







