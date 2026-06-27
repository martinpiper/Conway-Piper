cmake_minimum_required(VERSION 3.5)

SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED True)
#SET(GLEW_INCLUDE_DIRS "C:/Users/marti/Downloads/glew-1.9.0-win64/glew-1.9.0/include")
#SET(GLEW_LIBRARIES "C:/Users/marti/Downloads/glew-1.9.0-win64/glew-1.9.0/lib")
# add_compile_definitions(_MY_OPENGL_IS_33_)

project(VoxelRendering)

#find_package(Freetype REQUIRED)
#find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(dep/glfw)
add_subdirectory(dep/glm)

add_subdirectory(src)