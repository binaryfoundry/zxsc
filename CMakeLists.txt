cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME zxsc)
set(PROJECT_files_NAME zxsc-files)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(EMSCRIPTEN "Web Compilation" OFF)

set(SOURCES
    "src/Main.cpp")

set(HEADERS
    "src/Main.hpp")

set(SOURCES_SDL
    src/sdl/SDL.cpp
    src/sdl/SDLMain.cpp
    src/sdl/SDLMainWeb.cpp
    src/sdl/SDLFile.cpp)

set(HEADERS_SDL
    src/sdl/SDL.hpp
    src/sdl/SDLMain.hpp
    src/sdl/SDLMainWeb.hpp
    src/sdl/SDLFile.hpp)

set(SOURCES_SPECCY
    "src/speccy/Z80.c"
    "src/speccy/Rom.c"
    "src/speccy/Speccy.c"
    "src/speccy/Clock.c"
    "src/speccy/Memory.c"
    "src/speccy/Keyboard.c")

set(HEADERS_SPECCY
    "src/speccy/Z80.h"
    "src/speccy/Rom.h"
    "src/speccy/Speccy.h"
    "src/speccy/Clock.h"
    "src/speccy/Memory.h"
    "src/speccy/Keyboard.h")

SOURCE_GROUP("Source" FILES ${SOURCES})
SOURCE_GROUP("Source" FILES ${HEADERS})

SOURCE_GROUP("Source\\sdl" FILES ${SOURCES_SDL})
SOURCE_GROUP("Source\\sdl" FILES ${HEADERS_SDL})

SOURCE_GROUP("Source\\speccy" FILES ${SOURCES_SPECCY})
SOURCE_GROUP("Source\\speccy" FILES ${HEADERS_SPECCY})

include_directories(
    ${PROJECT_SOURCE_DIR}/src)

if (WIN32)

    message(STATUS "Platform: Windows")
    add_compile_options("/EHsc")
    include_directories(${PROJECT_SOURCE_DIR}/${EXTERNAL_DEPS_DIR}/sdl/win/include)
    link_directories(${PROJECT_SOURCE_DIR}/${EXTERNAL_DEPS_DIR}/sdl/win/lib/x64)

    set(LIBRARIES
        SDL2
        SDL2main)

elseif (EMSCRIPTEN)

    message(STATUS "Platform: HTML5")

endif ()

add_executable(
    ${PROJECT_NAME}
    MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${SOURCES_SDL}
    ${HEADERS_SDL}
    ${SOURCES_SPECCY}
    ${HEADERS_SPECCY})

target_link_libraries(
    ${PROJECT_NAME}
    ${LIBRARIES})

add_custom_target(
    ${PROJECT_files_NAME} ALL
    COMMENT "Copying Files..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/files/
    ${CMAKE_CURRENT_BINARY_DIR}/files)

add_dependencies(${PROJECT_NAME} ${PROJECT_files_NAME})

if (WIN32)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/lib/sdl/win/lib/x64/dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>)

endif ()

if (EMSCRIPTEN)

    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES SUFFIX ".html"
        LINK_FLAGS "--bind -s USE_SDL=2 -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s --emrun -std=c++11 -O3 --preload-file files --shell-file \"${PROJECT_SOURCE_DIR}/shell.html\" --use-preload-plugins")

endif ()
