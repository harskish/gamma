cmake_minimum_required(VERSION 3.0)
project(Gamma)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(Gamma/Vendor/glfw)

option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
add_subdirectory(Gamma/Vendor/assimp)

option(BUILD_BULLET2_DEMOS OFF)
option(BUILD_CPU_DEMOS OFF)
option(BUILD_EXTRAS OFF)
option(BUILD_OPENGL3_DEMOS OFF)
option(BUILD_UNIT_TESTS OFF)
add_subdirectory(Gamma/Vendor/bullet)

# Import CLT in compatibility mode (since VexCL requires cl.hpp)
add_definitions(-DCLT_CL_1_HEADER)
set(CLT_USE_CL_1_HEADER ON CACHE BOOL " " FORCE)
add_subdirectory(Gamma/Vendor/CLT)

#option(VEXCL_BACKEND "OpenCL")
add_definitions(-D__CL_ENABLE_EXCEPTIONS)
add_subdirectory(Gamma/Vendor/vexcl)

# Put all bullet targets into a separate folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(Bullet2FileLoader PROPERTIES FOLDER Bullet)
set_target_properties(Bullet3Collision PROPERTIES FOLDER Bullet)
set_target_properties(Bullet3Dynamics PROPERTIES FOLDER Bullet)
set_target_properties(Bullet3Geometry PROPERTIES FOLDER Bullet)
set_target_properties(Bullet3Common PROPERTIES FOLDER Bullet)
set_target_properties(Bullet3OpenCL_clew PROPERTIES FOLDER Bullet)
set_target_properties(BulletCollision PROPERTIES FOLDER Bullet)
set_target_properties(BulletDynamics PROPERTIES FOLDER Bullet)
set_target_properties(BulletInverseDynamics PROPERTIES FOLDER Bullet)
set_target_properties(BulletSoftBody PROPERTIES FOLDER Bullet)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

include_directories(Gamma/Sources/
                    Gamma/Vendor/assimp/include/
                    Gamma/Vendor/bullet/src/
                    Gamma/Vendor/glad/include/
                    Gamma/Vendor/glfw/include/
                    Gamma/Vendor/glm/
                    Gamma/Vendor/stb/
                    Gamma/Vendor/tinyfd/
                    Gamma/Vendor/imgui/
					Gamma/Vendor/xxhash/
                    Gamma/Vendor/vexcl/
					Gamma/Vendor/CLT/include/
                    ${CLT_CL_INCLUDE_DIR})

file(GLOB VENDORS_SOURCES Gamma/Vendor/glad/src/glad.c
                          Gamma/Vendor/tinyfd/tinyfiledialogs.c
                          Gamma/Vendor/imgui/*.cpp
						  Gamma/Vendor/xxhash/xxhash.c
						  Gamma/Sources/imgui_impl_glfw_gl3.cpp
						  Gamma/Sources/imgui_impl_glfw_gl3.h)
file(GLOB PROJECT_HEADERS Gamma/Sources/*.hpp
                          Gamma/Sources/Simulation/SPH/*.hpp
                          Gamma/Sources/Simulation/Particles/*.hpp)
file(GLOB PROJECT_SOURCES Gamma/Sources/*.cpp
                          Gamma/Sources/Simulation/SPH/*.cpp
                          Gamma/Sources/Simulation/Particles/*.cpp)
file(GLOB PROJECT_SHADERS Gamma/Shaders/*.comp
                          Gamma/Shaders/*.frag
                          Gamma/Shaders/*.geom
                          Gamma/Shaders/*.vert
						  Gamma/Shaders/*.glh)
file(GLOB PROJECT_CONFIGS CMakeLists.txt
                          README.md
                         .gitignore)

source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Shaders" FILES ${PROJECT_SHADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})

add_definitions(-DGLFW_INCLUDE_NONE
                -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
                               ${PROJECT_SHADERS} ${PROJECT_CONFIGS}
                               ${VENDORS_SOURCES})
target_link_libraries(${PROJECT_NAME} assimp glfw CLT
                      ${GLFW_LIBRARIES} ${GLAD_LIBRARIES}
                      BulletDynamics BulletCollision LinearMath
                      VexCL::OpenCL)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

# For relative paths    
set_target_properties(Gamma PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
