cmake_minimum_required(VERSION 3.15)
project(HubisClass CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project directories
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(IMGUI_DIR "${PROJECT_ROOT}/donanim/imgui-1.90.2")
set(GLFW_INCLUDE_DIR "${PROJECT_ROOT}/donanim/gl/include")
set(GLFW_LIB_DIR "${PROJECT_ROOT}/donanim/lib")
set(MINGW_BIN_DIR "${PROJECT_ROOT}/donanim/mingw64/bin")

# Include directories
include_directories(
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_INCLUDE_DIR}
)

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# GLFW library
set(GLFW_LIB "${GLFW_LIB_DIR}/libglfw3.a")

# OpenGL libraries (MinGW64)
set(OPENGL_LIBS "-lopengl32 -lgdi32")

# Executable
add_executable(HubisClass main.cpp ${IMGUI_SOURCES})

# Link libraries with static linking
target_link_libraries(HubisClass
    ${GLFW_LIB}
    ${OPENGL_LIBS}
    -static
    -static-libgcc
    -static-libstdc++
)

# Set output directory
set_target_properties(HubisClass PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}"
)
