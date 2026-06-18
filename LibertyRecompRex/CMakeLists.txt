project("LibertyRecompRex")

if (NOT WIN32)
    message(FATAL_ERROR "LibertyRecompRex is a Windows-only ReXGlue sidecar target.")
endif()

set(REXGLUE_ROOT "${CMAKE_SOURCE_DIR}/glue/rexglue-sdk-main")
set(REXGLUE_PREBUILT_DIR "${REXGLUE_ROOT}/out/win-amd64")
set(LIBERTY_REX_SIMDE_INCLUDE_DIR "${REXGLUE_ROOT}/thirdparty/simde")
if (NOT EXISTS "${LIBERTY_REX_SIMDE_INCLUDE_DIR}/simde/x86/avx.h")
    set(LIBERTY_REX_SIMDE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/tools/XenonRecomp/thirdparty")
endif()
if (NOT EXISTS "${LIBERTY_REX_SIMDE_INCLUDE_DIR}/simde/x86/avx.h")
    message(FATAL_ERROR
        "Missing SIMDe x86 headers for LibertyRecompRex; checked "
        "${REXGLUE_ROOT}/thirdparty/simde and "
        "${CMAKE_SOURCE_DIR}/tools/XenonRecomp/thirdparty")
endif()

set(LIBERTY_REX_REQUIRED_LIBS
    rexkernel
    rexsystem
    rexcore
    rexfilesystem
    fmt
    spdlog
    o1heap
    mspack
    xxhash
)

set(LIBERTY_REX_PREBUILT_LIBS)
foreach(_lib IN LISTS LIBERTY_REX_REQUIRED_LIBS)
    set(_path "${REXGLUE_PREBUILT_DIR}/${_lib}.lib")
    if (NOT EXISTS "${_path}")
        message(FATAL_ERROR
            "Missing ReXGlue prebuilt library for LibertyRecompRex: ${_path}")
    endif()
    list(APPEND LIBERTY_REX_PREBUILT_LIBS "${_path}")
endforeach()
unset(_path)
unset(_lib)

add_executable(LibertyRecompRex
    "src/pre_guest_import_bridges.cpp"
    "src/main.cpp"
)

target_compile_features(LibertyRecompRex PRIVATE cxx_std_23)
target_compile_definitions(LibertyRecompRex PRIVATE SPDLOG_COMPILED_LIB)

target_include_directories(LibertyRecompRex PRIVATE
    "${REXGLUE_ROOT}/include"
    "${REXGLUE_ROOT}/thirdparty/fmt/include"
    "${REXGLUE_ROOT}/thirdparty/spdlog/include"
    "${LIBERTY_REX_SIMDE_INCLUDE_DIR}"
    "${REXGLUE_ROOT}/thirdparty/tomlplusplus/include"
)

target_link_libraries(LibertyRecompRex PRIVATE
    LibertyRecompLib
    bcrypt
    ws2_32
)
