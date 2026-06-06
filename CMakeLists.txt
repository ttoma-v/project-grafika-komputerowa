cmake_minimum_required(VERSION 3.16)
project(UnderwaterBathyscaphe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

add_executable(underwater_bathy
    src/main.cpp
    src/Camera.cpp
    src/GlLoader.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Texture.cpp
    src/Geometry.cpp
    src/PTF.cpp
)

target_include_directories(underwater_bathy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(underwater_bathy PRIVATE
    glfw
    glm::glm
)

if(WIN32)
    target_link_libraries(underwater_bathy PRIVATE opengl32)
elseif(APPLE)
    find_library(OPENGL_LIB OpenGL)
    target_link_libraries(underwater_bathy PRIVATE ${OPENGL_LIB})
else()
    target_link_libraries(underwater_bathy PRIVATE GL)
endif()

target_compile_definitions(underwater_bathy PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    GLM_FORCE_RADIANS
    NOMINMAX
)

if(MSVC)
    target_compile_options(underwater_bathy PRIVATE /W4 /utf-8)
else()
    target_compile_options(underwater_bathy PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET underwater_bathy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:underwater_bathy>/assets
    COMMENT "Copying assets to build directory"
)
