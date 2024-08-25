cmake_minimum_required(VERSION 3.29)
project(ffmpeg_testbed)

# remux
add_executable(remux remux.cpp)
target_compile_features(remux PRIVATE cxx_std_23)
target_compile_options(remux PRIVATE /Wall /WX)

target_include_directories(remux PRIVATE vendor/ffmpeg/include)
target_link_directories(remux PRIVATE vendor/ffmpeg/lib)
target_link_libraries(remux PRIVATE avcodec avformat avutil)

add_custom_command(TARGET remux POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/vendor/ffmpeg/bin
        $<TARGET_FILE_DIR:remux>
)

# transcode
add_executable(transcode transcode.cpp)
target_compile_features(transcode PRIVATE cxx_std_23)
#target_compile_options(transcode PRIVATE /Wall /WX)

target_include_directories(transcode PRIVATE vendor/ffmpeg/include)
target_link_directories(transcode PRIVATE vendor/ffmpeg/lib)
target_link_libraries(transcode PRIVATE avcodec avformat avutil swscale swresample)

add_custom_command(TARGET transcode POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/vendor/ffmpeg/bin
        $<TARGET_FILE_DIR:transcode>
)