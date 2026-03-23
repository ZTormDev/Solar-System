cmake_minimum_required(VERSION 3.22)
project(SolarSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(SolarSystem
    src/main.cpp
    src/core/FileLoader.cpp
    src/scene/CubeMesh.cpp
    src/scene/Camera.cpp
    src/scene/Player.cpp
    src/scene/Scene.cpp
    src/scene/Transform.cpp
    src/renderer/Vertex.cpp
    src/renderer/VulkanApp.cpp
)

target_include_directories(SolarSystem PRIVATE src)

target_link_libraries(SolarSystem PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

if(MSVC)
    target_compile_options(SolarSystem PRIVATE /W4)
else()
    target_compile_options(SolarSystem PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

find_program(GLSLC glslc HINTS
    $ENV{VULKAN_SDK}/Bin
    $ENV{VULKAN_SDK}/Bin32
)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK and ensure VULKAN_SDK is set.")
endif()

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/cube.vert
    ${SHADER_SOURCE_DIR}/cube.frag
)

set(SHADER_BINARIES)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUT}
        DEPENDS ${SHADER}
        VERBATIM
    )
    list(APPEND SHADER_BINARIES ${SHADER_OUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_BINARIES})
add_dependencies(SolarSystem Shaders)
