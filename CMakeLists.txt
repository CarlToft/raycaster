cmake_minimum_required(VERSION 3.16)
project(raycaster)
set(CMAKE_CXX_STANDARD 17)

# For find SDL2 ( see https://stackoverflow.com/questions/54005991/what-happened-to-findsdl2-in-cmake )
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

# Win32 version works for vcpkg
if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
else()
    find_package(SDL2 REQUIRED)
endif()

add_executable(raycaster
        main.cpp)

# On windows, we need to link with SDL2::SDL2 and SDL2::SDL2main as the FindSDL2.cmake script does not work same as the vcpkg one.
if(WIN32)
    # Create a list of all the libraries we need to link with.
    list(APPEND SDL_LIBRARIES
        SDL2::SDL2
        SDL2::SDL2main)
else()
    list(APPEND SDL_LIBRARIES
        SDL2::Main)
endif()

# Debug message with the list of libraries we need to link with.
message(STATUS "Linking with libraries: ${SDL_LIBRARIES}")

target_link_libraries(raycaster
        ${SDL_LIBRARIES})

# Maybe there is a better way to do this?
file(GLOB TEXTURES ${CMAKE_CURRENT_SOURCE_DIR}/images/*.bmp)
file(COPY ${TEXTURES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/images/)