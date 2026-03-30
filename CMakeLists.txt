cmake_minimum_required(VERSION 3.16)
project(isdn3000e_lab8)

set(CMAKE_CXX_STANDARD 17)


add_executable(isdn3000e_lab8 main.cpp
        tasks/task1.cpp
        tasks/task1.h
        tasks/task2.cpp
        tasks/task2.h
        tasks/task3.cpp
        tasks/task3.h
        tasks/task4.cpp
        tasks/task4.h
        tasks/task5.cpp
        tasks/task5.h
)

find_package(OpenCV REQUIRED PATHS /usr/local/lib/cmake/opencv4 NO_DEFAULT_PATH)
find_package(realsense2 REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

target_link_libraries(isdn3000e_lab8
        PRIVATE
        ${OpenCV_LIBS}
        realsense2::realsense2)

target_include_directories(isdn3000e_lab8
        PRIVATE
        ${OpenCV_INCLUDE_DIRS})

add_custom_command(TARGET isdn3000e_lab8 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:isdn3000e_lab8>/data
)