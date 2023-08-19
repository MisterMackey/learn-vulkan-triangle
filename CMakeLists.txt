cmake_minimum_required(VERSION 3.2.0)
project(Triangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_link_options(-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi)

add_executable(Triangle main.cpp debugshit.cpp p_device.cpp requirement.cpp presentation.cpp)

add_test(NAME Triangle COMMAND ./Triangle)