cmake_minimum_required(VERSION 2.8)

project(CVASGMT1)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

find_package(Boost COMPONENTS system REQUIRED)
find_package (Threads)
find_package(OpenCV REQUIRED)

add_executable(app ${CMAKE_SOURCE_DIR}/src/app.cpp)

target_link_libraries(app 
${Boost_SYSTEM_LIBRARY}
${CMAKE_THREAD_LIBS_INIT}
${OpenCV_LIBS}
)