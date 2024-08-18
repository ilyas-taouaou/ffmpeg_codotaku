cmake_minimum_required(VERSION 3.29)
project(ffmpeg_testbed)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE vendor/ffmpeg/include)
target_link_directories(${PROJECT_NAME} PRIVATE vendor/ffmpeg/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec avformat avutil)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/vendor/ffmpeg/bin
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)