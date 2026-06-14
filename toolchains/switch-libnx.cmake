# Nintendo Switch / libnx (devkitPro) Toolchain
# Requirements:
#   - devkitPro with devkitA64 and libnx installed
#     Install: https://devkitpro.org/wiki/Getting_Started
#     Then:    dkp-pacman -S switch-dev
#   - Environment: DEVKITPRO and DEVKITA64 set by devkitPro's env.sh
#
# Setup:
#   source $DEVKITPRO/devkita64/environment-setup-aarch64-none-elf
#   cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/switch-libnx.cmake \
#         -DLIBERTY_RECOMP_TARGET_PLATFORM=switch \
#         -DLIBERTY_RECOMP_EMBEDDED_GAME_PATH=tools/local_game_payload \
#         ..
#
# Verify installed packages:
#   dkp-pacman -Q switch-dev

# Resolve devkitPro paths
if(NOT DEFINED DEVKITPRO)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO "$ENV{DEVKITPRO}")
    elseif(EXISTS "C:/")
        set(DEVKITPRO "C:/devkitPro")
    else()
        set(DEVKITPRO "/opt/devkitpro")  # common default
    endif()
endif()

if(NOT DEFINED DEVKITA64)
    if(DEFINED ENV{DEVKITA64})
        set(DEVKITA64 "$ENV{DEVKITA64}")
    else()
        set(DEVKITA64 "${DEVKITPRO}/devkitA64")
    endif()
endif()

if(NOT EXISTS "${DEVKITA64}")
    message(FATAL_ERROR
        "devkitA64 not found at: ${DEVKITA64}\n"
        "Install devkitPro: https://devkitpro.org/wiki/Getting_Started\n"
        "Then run: dkp-pacman -S switch-dev\n"
        "Then source devkitPro's environment script and re-run cmake.\n"
        "Verify: dkp-pacman -Q switch-dev")
endif()

message(STATUS "Using devkitA64: ${DEVKITA64}")
message(STATUS "Using devkitPro: ${DEVKITPRO}")

set(CMAKE_SYSTEM_NAME Switch)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(LIBERTY_RECOMP_SWITCH ON CACHE BOOL "Build Liberty Recomp for Nintendo Switch/libnx" FORCE)

set(_DKP_EXE_SUFFIX "")
if(EXISTS "${DEVKITA64}/bin/aarch64-none-elf-gcc.exe")
    set(_DKP_EXE_SUFFIX ".exe")
endif()

set(CMAKE_C_COMPILER   "${DEVKITA64}/bin/aarch64-none-elf-gcc${_DKP_EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-g++${_DKP_EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER "${DEVKITA64}/bin/aarch64-none-elf-gcc${_DKP_EXE_SUFFIX}")
set(CMAKE_AR           "${DEVKITA64}/bin/aarch64-none-elf-ar${_DKP_EXE_SUFFIX}")
set(CMAKE_RANLIB       "${DEVKITA64}/bin/aarch64-none-elf-ranlib${_DKP_EXE_SUFFIX}")
set(CMAKE_LINKER       "${DEVKITA64}/bin/aarch64-none-elf-ld${_DKP_EXE_SUFFIX}")
unset(_DKP_EXE_SUFFIX)

set(CMAKE_SYSROOT "${DEVKITA64}/aarch64-none-elf")

# libnx headers and libs
include_directories(SYSTEM
    "${DEVKITPRO}/portlibs/switch/include"
    "${DEVKITPRO}/portlibs/switch/include/SDL2"
    "${DEVKITPRO}/libnx/include"
    "${DEVKITA64}/aarch64-none-elf/include"
)
link_directories(
    "${DEVKITPRO}/portlibs/switch/lib"
    "${DEVKITPRO}/libnx/lib"
    "${DEVKITA64}/aarch64-none-elf/lib/pic"
    "${DEVKITA64}/aarch64-none-elf/lib"
)

list(APPEND CMAKE_PREFIX_PATH "${DEVKITPRO}/portlibs/switch")
set(CMAKE_FIND_ROOT_PATH
    "${DEVKITPRO}/portlibs/switch"
    "${DEVKITPRO}/libnx"
    "${DEVKITA64}/aarch64-none-elf")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ZLIB_INCLUDE_DIR "${DEVKITPRO}/portlibs/switch/include" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY "${DEVKITPRO}/portlibs/switch/lib/libz.a" CACHE FILEPATH "" FORCE)
set(CURL_INCLUDE_DIR "${DEVKITPRO}/portlibs/switch/include" CACHE PATH "" FORCE)
set(CURL_LIBRARY "${DEVKITPRO}/portlibs/switch/lib/libcurl.a" CACHE FILEPATH "" FORCE)
set(DIRECTX_DXC_TOOL
    "C:/Users/Jellybone/Documents/GitHub/LibertyRecomp/tools/XenosRecomp/thirdparty/dxc-bin/bin/x64/dxc.exe"
    CACHE FILEPATH "Host DXC executable for shader header generation" FORCE)

# Compile flags for Switch homebrew
set(SWITCH_C_FLAGS   "-ffunction-sections -fdata-sections -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -D__SWITCH__ -DNX -DSPDLOG_NO_TZ_OFFSET -Wno-psabi")
set(SWITCH_CXX_FLAGS "${SWITCH_C_FLAGS}")

set(CMAKE_C_FLAGS_INIT   "${SWITCH_C_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${SWITCH_CXX_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${SWITCH_C_FLAGS}")

set(LIBERTY_SWITCH_LD_SCRIPT "${CMAKE_BINARY_DIR}/liberty-switch.ld" CACHE FILEPATH "Build-local copy of the libnx Switch linker script" FORCE)
configure_file("${DEVKITPRO}/libnx/switch.ld" "${LIBERTY_SWITCH_LD_SCRIPT}" COPYONLY)

set(LIBERTY_SWITCH_SPECS "${CMAKE_BINARY_DIR}/liberty-switch.specs" CACHE FILEPATH "Generated Switch specs file with absolute devkitPro paths" FORCE)
file(WRITE "${LIBERTY_SWITCH_SPECS}"
"*link:
+ -T ${LIBERTY_SWITCH_LD_SCRIPT} -pie --no-dynamic-linker --spare-dynamic-tags=0 --gc-sections -z text -z now -z gcs=never -z nodynamic-undefined-weak -z pack-relative-relocs --build-id=sha1 --nx-module-name

*startfile:
crti%O%s crtbegin%O%s --require-defined=main
")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=${LIBERTY_SWITCH_SPECS} -L${DEVKITPRO}/portlibs/switch/lib -L${DEVKITPRO}/libnx/lib -L${DEVKITA64}/aarch64-none-elf/lib/pic -L${DEVKITA64}/aarch64-none-elf/lib -Wl,--gc-sections")

# Switch uses Vulkan via deko3d or SDL2's libnx backend
set(LIBERTY_RECOMP_VULKAN ON CACHE BOOL "Use Vulkan/deko3d renderer" FORCE)
set(LIBERTY_RECOMP_D3D12 OFF CACHE BOOL "" FORCE)
set(LIBERTY_RECOMP_METAL OFF CACHE BOOL "" FORCE)

# No installer wizard on Switch
set(LIBERTY_RECOMP_EMBEDDED_ASSETS ON CACHE BOOL "Package assets at build time (no installer UI)" FORCE)

# NRO (homebrew) output settings
set(LIBERTY_SWITCH_APP_TITLE "Liberty Recomp" CACHE STRING "Switch homebrew title")
set(LIBERTY_SWITCH_APP_AUTHOR "LibertyRecomp Team" CACHE STRING "Switch homebrew author")
set(LIBERTY_SWITCH_APP_VERSION "1.0.0" CACHE STRING "Switch homebrew version")
