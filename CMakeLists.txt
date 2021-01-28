cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME zxs)
set(PROJECT_files_NAME zxs-files)
set(EXTERNAL_DEPS_DIR "lib")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src/")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 11)

option(EMSCRIPTEN "Web Compilation" OFF)

if (NOT EMSCRIPTEN)
    if (${APPLE})
        set(MACOSX TRUE)
    endif()
endif()

project(${PROJECT_NAME} C CXX)
enable_language(CXX)
enable_language(C)

set(SOURCES
    "src/Main.cpp"
    "src/System.cpp")

set(HEADERS
    "src/Main.hpp"
    "src/System.hpp")

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

elseif (MACOSX)

    message(STATUS "Platform: Darwin (Mac OS X)")

    # xcrun --show-sdk-path
    set(SDKVER "10.14")
    set(SDKROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk")

    if(EXISTS ${SDKROOT})
        set(CMAKE_OSX_SYSROOT "${SDKROOT}")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${SDKVER}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${SDKVER}")
    else()
        message(FATAL_ERROR "Error: Mac OS X SDK ${SDKVER} not found at: ${SDKROOT}")
    endif()

    set(FRAMEWORKS
        Foundation)

    foreach(FW ${FRAMEWORKS})
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework ${FW}")
    endforeach()

    set(LIBRARIES
        "${PROJECT_SOURCE_DIR}/${EXTERNAL_DEPS_DIR}/sdl/osx/SDL2.framework")

    set(LIBRARIES_HEADERS
        "${PROJECT_SOURCE_DIR}/${EXTERNAL_DEPS_DIR}/sdl/osx/SDL2.framework/Headers")

elseif (EMSCRIPTEN)

    message(STATUS "Platform: HTML5")

endif ()

add_executable(
    ${PROJECT_NAME}
    MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
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
        LINK_FLAGS "--bind -s USE_SDL=2 -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 -s --emrun -std=c++11 -O3 --preload-file files --shell-file \"${PROJECT_SOURCE_DIR}/shell.html\" --use-preload-plugins")

endif ()
