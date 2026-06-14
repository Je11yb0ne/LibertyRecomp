set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SNAPPY_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(SNAPPY_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)

add_subdirectory(fmt)
add_subdirectory(spdlog)
add_subdirectory(tomlplusplus)
add_subdirectory(utfcpp)
if(NOT REXGLUE_CODEGEN_ONLY)
    add_subdirectory(snappy)
    add_subdirectory(sdl2)
endif()

if(TARGET tomlplusplus::tomlplusplus AND NOT TARGET tomlplusplus)
    add_library(tomlplusplus INTERFACE)
    target_link_libraries(tomlplusplus INTERFACE tomlplusplus::tomlplusplus)
endif()

add_library(simde INTERFACE)
target_include_directories(simde INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/simde
    ${REXGLUE_ROOT}/../../tools/XenonRecomp/thirdparty
)

add_library(disruptorplus INTERFACE)

add_library(renderdoc INTERFACE)

add_subdirectory(${REXGLUE_ROOT}/../../thirdparty/o1heap o1heap)

add_library(aes128 INTERFACE)

add_library(mspack STATIC
    libmspack/libmspack/mspack/system.c
    libmspack/libmspack/mspack/lzxd.c
)
target_include_directories(mspack PUBLIC libmspack/libmspack/mspack)

add_library(xxhash STATIC xxHash/xxhash.c)
add_library(xxHash::xxhash ALIAS xxhash)
target_include_directories(xxhash PUBLIC xxHash)

add_subdirectory(${REXGLUE_ROOT}/../../tools/XenonRecomp/thirdparty/disasm disasm)

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)
add_library(imgui::imgui ALIAS imgui)
target_include_directories(imgui PUBLIC imgui)

add_library(dxc-headers INTERFACE)

if(NOT REXGLUE_CODEGEN_ONLY AND REXGLUE_USE_VULKAN)
    add_subdirectory(vulkan-headers)
    add_subdirectory(volk)
    if(TARGET volk)
        target_include_directories(volk PUBLIC vulkan-headers/include)
    endif()
    add_subdirectory(vulkan-memory-allocator)
    add_subdirectory(spirv-headers)
    add_subdirectory(spirv-tools)
    add_subdirectory(glslang)
    if(TARGET SPIRV AND NOT TARGET glslang::SPIRV)
        add_library(glslang::SPIRV ALIAS SPIRV)
    endif()
endif()
