cmake_minimum_required(VERSION 3.10)

project(R-Type)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_DEBUG_POSTFIX d)


# set(VCPKG_SHOULD_NOT_UPDATE 1)
include(../../cmake/automate-vcpkg.cmake)
vcpkg_bootstrap()
vcpkg_install_packages(raylib stduuid nlohmann-json)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        add_compile_options(/D_ITERATOR_DEBUG_LEVEL=0)
    endif()
    # The static permits to enter the functions, avoid settings the path to a .dll etc...
    find_library(GEngine_LIB NAMES GEngined PATHS ../../build/Debug ../../build)
    # find_library(GEngine_LIB NAMES GEngined PATHS ../../build/Debug ../../build)
else()
    find_library(GEngine_LIB NAMES GEngined PATHS ../../build/Release ../../build)
endif()

file(GLOB_RECURSE SOURCES source/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)

find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

include_directories(include)
include_directories(../../include)

add_executable(poc3d_client ${SOURCES} ${HEADERS})
add_executable(poc3d_server ${SOURCES} ${HEADERS})

if(WIN32)
target_link_libraries(poc3d_client PRIVATE ${GEngine_LIB} raylib wsock32 ws2_32)
target_link_libraries(poc3d_server PRIVATE ${GEngine_LIB} raylib wsock32 ws2_32)
else()
target_link_libraries(poc3d_client PRIVATE ${GEngine_LIB} raylib glfw)
target_link_libraries(poc3d_server PRIVATE ${GEngine_LIB} raylib glfw)
endif()

set_target_properties(poc3d_client PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)
set_target_properties(poc3d_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)

install(TARGETS poc3d_client poc3d_server
    RUNTIME DESTINATION bin
)

target_compile_definitions(poc3d_server PRIVATE GEngine_Server)
target_compile_definitions(poc3d_client PRIVATE GEngine_Client)
