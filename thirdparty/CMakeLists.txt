cmake_policy(SET CMP0077 NEW)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)
set(MSDFGEN_DISABLE_PNG ON)

set(SDL2MIXER_DEPS_SHARED OFF)
set(SDL2MIXER_VENDORED ON)
set(SDL2MIXER_FLAC OFF)
set(SDL2MIXER_MOD OFF)
set(SDL2MIXER_MP3 OFF)
set(SDL2MIXER_MIDI OFF)
set(SDL2MIXER_OPUS OFF)
set(SDL2MIXER_VORBIS "VORBISFILE")
set(SDL2MIXER_WAVPACK OFF)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(SDL_VULKAN_ENABLED ON CACHE BOOL "")
endif()

if (WIN32)
    set(D3D12_AGILITY_SDK_ENABLED ON CACHE BOOL "")
endif()

if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(BASE_ARCHITECTURE "${CMAKE_OSX_ARCHITECTURES}")
elseif (CMAKE_SYSTEM_PROCESSOR)
    set(BASE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(BASE_ARCHITECTURE "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

# Next, match common architecture strings down to a known common value.
if (BASE_ARCHITECTURE MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(ARCHITECTURE "x86_64")
elseif (BASE_ARCHITECTURE MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set(ARCHITECTURE "arm64")
else()
    message(FATAL_ERROR "Unsupported CPU architecture: ${BASE_ARCHITECTURE}")
endif()

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND CMAKE_PREFIX_PATH "${DEVKITPRO}/portlibs/switch")
    find_package(SDL2 CONFIG REQUIRED)

    if (NOT TARGET SDL2::SDL2-static)
        add_library(SDL2::SDL2-static STATIC IMPORTED GLOBAL)
        set_target_properties(SDL2::SDL2-static PROPERTIES
            IMPORTED_LOCATION "${DEVKITPRO}/portlibs/switch/lib/libSDL2.a"
            INTERFACE_INCLUDE_DIRECTORIES "${DEVKITPRO}/portlibs/switch/include/SDL2"
            INTERFACE_LINK_LIBRARIES "EGL;stdc++;glapi;drm_nouveau;nx;pthread"
            INTERFACE_LINK_DIRECTORIES "${DEVKITPRO}/portlibs/switch/lib;${DEVKITPRO}/libnx/lib")
    endif()

    if (NOT TARGET SDL2_mixer)
        add_library(SDL2_mixer STATIC IMPORTED GLOBAL)
        set_target_properties(SDL2_mixer PROPERTIES
            IMPORTED_LOCATION "${DEVKITPRO}/portlibs/switch/lib/libSDL2_mixer.a"
            INTERFACE_INCLUDE_DIRECTORIES "${DEVKITPRO}/portlibs/switch/include/SDL2"
            INTERFACE_LINK_LIBRARIES "${DEVKITPRO}/portlibs/switch/lib/libSDL2.a;EGL;stdc++;glapi;drm_nouveau;nx;pthread;vorbisidec;modplug;mpg123;opusfile;opus;ogg;FLAC;z"
            INTERFACE_LINK_DIRECTORIES "${DEVKITPRO}/portlibs/switch/lib;${DEVKITPRO}/libnx/lib")
    endif()
else()
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/msdf-atlas-gen")
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/SDL")
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/SDL_mixer")
endif()
if (NOT LIBERTY_RECOMP_SWITCH)
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/nativefiledialog-extended")
endif()
add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/o1heap")
add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/plume")

if (NOT LIBERTY_RECOMP_SWITCH)
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/ffmpeg-core")
endif()

if (APPLE)
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/MoltenVK")
endif()

if (NOT LIBERTY_RECOMP_SWITCH)
    # GameNetworkingSockets for P2P multiplayer
    set(BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_ICE ON CACHE BOOL "" FORCE)
    set(USE_STEAMWEBRTC OFF CACHE BOOL "" FORCE)
    set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
    add_subdirectory("${LIBERTY_RECOMP_THIRDPARTY_ROOT}/GameNetworkingSockets")
endif()
