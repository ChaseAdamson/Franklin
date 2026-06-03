cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE "C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(FDRender VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find packages ────────────────────────────────────────────
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# ── GLAD (bundled in third_party/) ───────────────────────────
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# ── Sources ───────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/renderer.cpp
    src/shaders.cpp
    src/gpu_resources.cpp
    src/geometry.cpp
    src/fdr_parser.cpp
)

add_executable(FDRender ${SOURCES})

target_include_directories(FDRender PRIVATE include)

target_link_libraries(FDRender
    PRIVATE
    OpenGL::GL
    OpenGL::GLU
    glfw
    glm::glm
    glad
)

# ── Copy assets to build directory ───────────────────────────
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
