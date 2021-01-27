cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME zxs)
set(EXTERNAL_DEPS_DIR "lib")
set(RESOURCES_DIR "resources")
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
    "src/System.cpp"
    "src/Z80.c"
    "src/Rom.c"
    "src/Speccy.c"
    "src/Mem.c")

set(HEADERS
    "src/Main.hpp"
    "src/System.hpp"
    "src/Z80.h"
    "src/Rom.h"
    "src/Speccy.h"
    "src/Mem.h")

SOURCE_GROUP("Source" FILES ${SOURCES})
SOURCE_GROUP("Source" FILES ${HEADERS})

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
    ${HEADERS})

target_link_libraries(
    ${PROJECT_NAME}
    ${LIBRARIES})

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
        LINK_FLAGS "--bind -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s USE_SDL=2 -s WASM=1 -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 -s --emrun -std=c++11 -O3 --use-preload-plugins")
    #--preload-file resources | -s USE_WEBGL2=1 | -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 |  -s DEMANGLE_SUPPORT=1 -s \"BINARYEN_METHOD='native-wasm'\"

endif ()
