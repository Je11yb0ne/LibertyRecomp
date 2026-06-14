project("LibertyRecomp")

if (WIN32)
    option(LIBERTY_RECOMP_D3D12 "Add D3D12 support for rendering" ON)
endif()

if (APPLE)
    option(LIBERTY_RECOMP_METAL "Add Metal support for rendering" ON)
endif()

if (LIBERTY_RECOMP_SWITCH)
    option(LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT "Enable the Switch sparse guest-memory audit backend." OFF)
    option(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP "Stop the Switch audit build after guest-memory mapping." OFF)
    option(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS
        "Skip generated function-table initialization in the Switch sparse memory audit backend."
        ${LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP})
    set(LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE "0x4000000"
        CACHE STRING "Low guest-memory window size for the Switch sparse memory audit.")
    set(LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE "0x2000000"
        CACHE STRING "Physical guest heap window size for the Switch sparse memory audit.")
    set(LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE ""
        CACHE STRING "Optional image/function-table window size override for the Switch sparse memory audit.")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(LIBERTY_RECOMP_FLATPAK "Configure the build for Flatpak compatibility." OFF)
endif()

function(BIN2C)
    cmake_parse_arguments(BIN2C_ARGS "" "TARGET_OBJ;SOURCE_FILE;DEST_FILE;ARRAY_NAME;COMPRESSION_TYPE" "" ${ARGN})

    if(NOT BIN2C_ARGS_TARGET_OBJ)
        message(FATAL_ERROR "TARGET_OBJ not specified.")
    endif()

    if(NOT BIN2C_ARGS_SOURCE_FILE)
        message(FATAL_ERROR "SOURCE_FILE not specified.")
    endif()

    if(NOT BIN2C_ARGS_DEST_FILE)
        set(BIN2C_ARGS_DEST_FILE "${BIN2C_ARGS_SOURCE_FILE}")
    endif()

    if(NOT BIN2C_ARGS_COMPRESSION_TYPE)
        set(BIN2C_ARGS_COMPRESSION_TYPE "none")
    endif()

    add_custom_command(OUTPUT "${BIN2C_ARGS_DEST_FILE}.c"
        COMMAND $<TARGET_FILE:file_to_c> "${BIN2C_ARGS_SOURCE_FILE}" "${BIN2C_ARGS_ARRAY_NAME}" "${BIN2C_ARGS_COMPRESSION_TYPE}" "${BIN2C_ARGS_DEST_FILE}.c" "${BIN2C_ARGS_DEST_FILE}.h"
        DEPENDS "${BIN2C_ARGS_SOURCE_FILE}"
        BYPRODUCTS "${BIN2C_ARGS_DEST_FILE}.h"
        COMMENT "Generating binary header for ${BIN2C_ARGS_SOURCE_FILE}..."
    )

    set_source_files_properties(${BIN2C_ARGS_DEST_FILE}.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    target_sources(${BIN2C_ARGS_TARGET_OBJ} PRIVATE ${BIN2C_ARGS_DEST_FILE}.c)
endfunction()

add_compile_options(
    -fno-strict-aliasing
    -Wno-switch
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-void-pointer-to-int-cast
    -Wno-int-to-void-pointer-cast
    -Wno-invalid-offsetof
    -Wno-null-arithmetic
    -Wno-null-conversion
    -Wno-tautological-undefined-compare
)

if (WIN32)
    add_compile_options(/fp:strict)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-ffp-model=strict)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-frounding-math -fsignaling-nans)
endif()

add_compile_definitions(
    SDL_MAIN_HANDLED
    _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR # Microsoft wtf?
    _CRT_SECURE_NO_WARNINGS)

set(LIBERTY_RECOMP_PRECOMPILED_HEADERS
    "stdafx.h"
)

set(LIBERTY_RECOMP_KERNEL_CXX_SOURCES
    "kernel/imports.cpp"
    "kernel/xdm.cpp"
    "kernel/heap.cpp"
    "kernel/memory.cpp"
    "kernel/xenon_memory.cpp"
    "kernel/xam.cpp"
    "kernel/save_system.cpp"
    "kernel/save_hooks.cpp"
    "kernel/vfs.cpp"
    "kernel/mapped_file.cpp"
    "kernel/io/file_system.cpp"
    "kernel/io/gta_file_system.cpp"
    "kernel/io/net_socket.cpp"
    "kernel/io/net_session.cpp"
    "kernel/io/session_tracker.cpp"
    "kernel/io/session_tracker_community.cpp"
    "kernel/io/session_tracker_firebase.cpp"
    "kernel/io/session_tracker_lan.cpp"
    "kernel/io/p2p_manager.cpp"
    "kernel/io/signaling_client.cpp"
    "kernel/game_init.cpp"
    "kernel/sync_table.cpp"
)

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_KERNEL_CXX_SOURCES "kernel/io/p2p_manager_switch_stub.cpp")
endif()

set(LIBERTY_RECOMP_LOCALE_CXX_SOURCES
    "locale/config_locale.cpp"
    "locale/locale.cpp"
)

if (LIBERTY_RECOMP_SWITCH)
    set(LIBERTY_RECOMP_OS_CXX_SOURCES
        "os/switch/logger_switch.cpp"
        "os/switch/media_switch.cpp"
        "os/switch/process_switch.cpp"
        "os/switch/user_switch.cpp"
        "os/switch/version_switch.cpp"
        "os/switch/audit_diagnostics_switch.cpp"
    )
    set_source_files_properties("os/switch/audit_diagnostics_switch.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
elseif (WIN32)
    set(LIBERTY_RECOMP_OS_CXX_SOURCES
        "os/win32/logger_win32.cpp"
        "os/win32/media_win32.cpp"
        "os/win32/process_win32.cpp"
        "os/win32/user_win32.cpp"
        "os/win32/version_win32.cpp"
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LIBERTY_RECOMP_OS_CXX_SOURCES
        "os/linux/logger_linux.cpp"
        "os/linux/media_linux.cpp"
        "os/linux/process_linux.cpp"
        "os/linux/user_linux.cpp"
        "os/linux/version_linux.cpp"
    )
elseif (APPLE)
    set(LIBERTY_RECOMP_OS_CXX_SOURCES
        "os/macos/logger_macos.cpp"
        "os/macos/media_macos.cpp"
        "os/macos/process_macos.cpp"
        "os/macos/user_macos.cpp"
        "os/macos/version_macos.cpp"
    )
endif()

set(LIBERTY_RECOMP_CPU_CXX_SOURCES
    "cpu/guest_thread.cpp"
)

set(LIBERTY_RECOMP_GPU_CXX_SOURCES
    "gpu/video.cpp"
    "gpu/imgui/imgui_common.cpp"
    "gpu/imgui/imgui_snapshot.cpp"
)

if (NOT LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_GPU_CXX_SOURCES "gpu/imgui/imgui_font_builder.cpp")
endif()

set(LIBERTY_RECOMP_APU_CXX_SOURCES
    "apu/audio.cpp"
    "apu/embedded_player.cpp"
    "apu/driver/sdl2_driver.cpp"
)

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_APU_CXX_SOURCES "apu/xma_decoder_switch_stub.cpp")
else()
    list(APPEND LIBERTY_RECOMP_APU_CXX_SOURCES "apu/xma_decoder.cpp")
endif()

set(LIBERTY_RECOMP_HID_CXX_SOURCES
    "hid/hid.cpp"
    "hid/mouse_camera.cpp"
    "hid/driver/sdl_hid.cpp"
)

set(LIBERTY_RECOMP_PATCHES_CXX_SOURCES
    # GTA IV-specific patches (active)
    "patches/gta4_patches.cpp"
    "patches/gta4_aspect_ratio_patches.cpp"
    "patches/gta4_input_patches.cpp"  # GTA IV input remapping
    "patches/audio_patches.cpp"       # GTA IV audio system (updated for RAGE)
    "patches/memcpy_patches.cpp"  # Native memcpy implementation to fix broken recompiled control flow
    "patches/player_limit_patches.cpp"  # Extended multiplayer - raise player limit from 16 to 32+

    # Generic/stub patches needed for compilation
    # "patches/aspect_ratio_patches.cpp"  # Has Sonicteam references - replaced by gta4_aspect_ratio_patches.cpp
    # "patches/camera_patches.cpp"         # Has Sonicteam references - needs GTA IV version
    # "patches/fps_patches.cpp"            # May need review
    # "patches/frontend_listener.cpp"      # May need review
    # "patches/input_patches.cpp"          # May need review
    # "patches/loading_patches.cpp"        # Has Sonicteam references
    # "patches/MainMenuTask_patches.cpp"   # Has Sonicteam references
    # "patches/misc_patches.cpp"           # May need review
    # "patches/pause_patches.cpp"          # Has Sonicteam references
    # "patches/player_patches.cpp"         # Has Sonicteam references
    # "patches/SaveDataTask_patches.cpp"   # Has Sonicteam references
    # "patches/text_patches.cpp"           # Has Sonicteam references
    # "patches/TitleTask_patches.cpp"      # Has Sonicteam references
    # "patches/video_patches.cpp"          # May need review
)

set(LIBERTY_RECOMP_UI_CXX_SOURCES
    "ui/achievement_menu.cpp"
    "ui/achievement_overlay.cpp"
    "ui/black_bar.cpp"
    "ui/button_window.cpp"
    "ui/common_menu.cpp"
    "ui/fader.cpp"
    "ui/game_window.cpp"
    "ui/imgui_utils.cpp"
    "ui/message_window_stub.cpp"   # Stub - GTA IV version needed
    "ui/network_status_overlay.cpp"
    "ui/options_menu_stub.cpp"     # Stub - GTA IV version needed
)

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_UI_CXX_SOURCES "ui/installer_wizard_switch_stub.cpp")
else()
    list(APPEND LIBERTY_RECOMP_UI_CXX_SOURCES "ui/installer_wizard.cpp")
endif()

set(LIBERTY_RECOMP_INSTALL_CXX_SOURCES
    "install/platform_paths.cpp"
    "install/update_checker.cpp"
)

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_INSTALL_CXX_SOURCES
        "install/embedded_assets.cpp"
        "install/installer_switch_stub.cpp")
else()
    list(APPEND LIBERTY_RECOMP_INSTALL_CXX_SOURCES
        "install/installer.cpp"
        "install/iso_file_system.cpp"
        "install/iso_extractor.cpp"
        "install/rpf_extractor.cpp"
        "install/shader_converter.cpp"
        "install/thread_pool.cpp"
        "install/xcontent_file_system.cpp"
        "install/xbox360/stfs_parser.cpp"
        "install/xbox360/xex_patcher.cpp"
        "install/xbox360/title_update_manager.cpp"
        "install/hashes/game.cpp"
        "install/hashes/dlc_tlad.cpp"
        "install/hashes/dlc_tbogt.cpp"
        "install/hashes/title_update.cpp")
endif()

set(LIBERTY_RECOMP_USER_CXX_SOURCES
    "user/achievement_data.cpp"
    "user/achievement_manager.cpp"
    "user/config.cpp"
    "user/persistent_storage_manager.cpp"  # GTA IV save data handling
    "user/registry.cpp"
    "user/paths.cpp"
)

set(LIBERTY_RECOMP_MOD_CXX_SOURCES
    "mod/mod_loader.cpp"
)

set(LIBERTY_RECOMP_UTILS_CXX_SOURCES
    "utils/bit_stream.cpp"
    "utils/ring_buffer.cpp"
)

if (LIBERTY_RECOMP_SWITCH)
    list(APPEND LIBERTY_RECOMP_UTILS_CXX_SOURCES
        "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/XenonUtils/image.cpp"
        "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/XenonUtils/xex.cpp"
        "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/XenonUtils/xdbf_wrapper.cpp"
        "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty/tiny-AES-c/aes.c"
    )
    set_source_files_properties("${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty/tiny-AES-c/aes.c" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

set(LIBERTY_RECOMP_THIRDPARTY_SOURCES
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/backends/imgui_impl_sdl2.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/imgui.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_demo.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_draw.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_tables.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui/imgui_widgets.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/implot/implot.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/implot/implot_demo.cpp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/implot/implot_items.cpp"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenosRecomp/thirdparty/smol-v/source/smolv.cpp"
)

set(LIBERTY_RECOMP_THIRDPARTY_INCLUDES
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/concurrentqueue"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/ddspp"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/imgui"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/implot"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/json/include"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/magic_enum/include"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/stb"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/unordered_dense/include"
    "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/GameNetworkingSockets/include"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/bc_diff"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenosRecomp/thirdparty/smol-v/source"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenosRecomp/thirdparty/zstd/lib"
)

set_source_files_properties(${LIBERTY_RECOMP_THIRDPARTY_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

set(LIBERTY_RECOMP_CXX_SOURCES
    "app.cpp"
    "exports.cpp"
    "main.cpp"
    "misc_impl.cpp"
    "preload_executable.cpp"
    "sdl_listener.cpp"
    "stdafx.cpp"
    "version.cpp"

    ${LIBERTY_RECOMP_KERNEL_CXX_SOURCES}
    ${LIBERTY_RECOMP_LOCALE_CXX_SOURCES}
    ${LIBERTY_RECOMP_OS_CXX_SOURCES}
    ${LIBERTY_RECOMP_CPU_CXX_SOURCES}
    ${LIBERTY_RECOMP_GPU_CXX_SOURCES}
    ${LIBERTY_RECOMP_APU_CXX_SOURCES}
    ${LIBERTY_RECOMP_HID_CXX_SOURCES}
    ${LIBERTY_RECOMP_PATCHES_CXX_SOURCES}
    ${LIBERTY_RECOMP_UI_CXX_SOURCES}
    ${LIBERTY_RECOMP_INSTALL_CXX_SOURCES}
    ${LIBERTY_RECOMP_USER_CXX_SOURCES}
    ${LIBERTY_RECOMP_UTILS_CXX_SOURCES}
    ${LIBERTY_RECOMP_MOD_CXX_SOURCES}
    ${LIBERTY_RECOMP_THIRDPARTY_SOURCES}
)

include("version.cmake")

set(VERSION_TXT "${PROJECT_SOURCE_DIR}/res/version.txt")

# Only show Git info and build type if not Release.
set(SHOW_GIT_INFO_AND_BUILD_TYPE 0)
if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Release")
    set(SHOW_GIT_INFO_AND_BUILD_TYPE 1)
endif()

if (LIBERTY_RECOMP_METAL)
    set(XCRUN_TOOL "/usr/bin/xcrun")
endif()

GenerateVersionSources(
    OUTPUT_DIR ${PROJECT_SOURCE_DIR}
    VERSION_TXT ${VERSION_TXT}
    H_TEMPLATE "${PROJECT_SOURCE_DIR}/res/version.h.template"
    CXX_TEMPLATE "${PROJECT_SOURCE_DIR}/res/version.cpp.template"
    BUILD_TYPE ${CMAKE_BUILD_TYPE}
    SHOW_GIT_INFO ${SHOW_GIT_INFO_AND_BUILD_TYPE}
    SHOW_BUILD_TYPE ${SHOW_GIT_INFO_AND_BUILD_TYPE}
)

if (WIN32)
    # Create binary version number for Win32 integer attributes.
    CreateVersionString(
        VERSION_TXT ${VERSION_TXT}
        OUTPUT_CSV 1
        OUTPUT_VAR WIN32_VERSION_BINARY
    )

    # Create string version number for Win32 detailed attributes.
    CreateVersionString(
        VERSION_TXT ${VERSION_TXT}
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        SHOW_GIT_INFO ${SHOW_GIT_INFO_AND_BUILD_TYPE}
        SHOW_BUILD_TYPE ${SHOW_GIT_INFO_AND_BUILD_TYPE}
        OUTPUT_VAR WIN32_VERSION_STRING
    )

    # Set Win32 icon path.
    set(WIN32_ICON_PATH "${PROJECT_SOURCE_DIR}/../LibertyRecompResources/images/game_icon.ico")

    configure_file("res/win32/res.rc.template" "${CMAKE_BINARY_DIR}/res.rc" @ONLY)
    add_executable(LibertyRecomp ${LIBERTY_RECOMP_CXX_SOURCES} "${CMAKE_BINARY_DIR}/res.rc")

    # Hide console for release configurations.
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        target_link_options(LibertyRecomp PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    endif()
elseif (APPLE)
    # Create version number for app bundle.
    CreateVersionString(
            VERSION_TXT ${VERSION_TXT}
            OUTPUT_VAR MACOS_BUNDLE_VERSION
    )

    add_executable(LibertyRecomp MACOSX_BUNDLE
            ${LIBERTY_RECOMP_CXX_SOURCES}
            res/macos/game_icon.icns
    )
    set_source_files_properties(res/macos/game_icon.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(LibertyRecomp PROPERTIES
            OUTPUT_NAME "Liberty Recompiled"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/res/macos/MacOSXBundleInfo.plist.in
            MACOSX_BUNDLE_GUI_IDENTIFIER OZORDI.LibertyRecomp
            MACOSX_BUNDLE_BUNDLE_NAME "Liberty Recompiled"
            MACOSX_BUNDLE_BUNDLE_VERSION ${MACOS_BUNDLE_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOS_BUNDLE_VERSION}
            MACOSX_BUNDLE_ICON_FILE "game_icon.icns"
    )

    # Linking with MoltenVK directly would prevent using the system Vulkan loader to load with debug layers.
    # Instead, copy the MoltenVK dylib to the app bundle along with an ICD file for the loader to find it.
    # In the event the loader is not installed, the MoltenVK dylib can still be picked up directly in the app bundle.
    set(MVK_BUNDLED_PATH "Resources/vulkan/icd.d")
    set(MVK_DST "${CMAKE_CURRENT_BINARY_DIR}/Liberty Recompiled.app/Contents/${MVK_BUNDLED_PATH}")
    set_property(TARGET LibertyRecomp APPEND PROPERTY BUILD_RPATH "@executable_path/../${MVK_BUNDLED_PATH}")

    set(MVK_ICD_SRC "${LIBERTY_RECOMP_THIRDPARTY_ROOT}/MoltenVK/MoltenVK/MoltenVK/icd/MoltenVK_icd.json")
    set(MVK_ICD_DST "${MVK_DST}/MoltenVK_icd.json")
    set(MVK_DYLIB_SRC "${CMAKE_BINARY_DIR}/thirdparty/MoltenVK/libMoltenVK.dylib")
    set(MVK_DYLIB_DST "${MVK_DST}/libMoltenVK.dylib")

    add_custom_command(
            OUTPUT ${MVK_DST}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${MVK_DST})
    add_custom_command(
            OUTPUT ${MVK_ICD_DST}
            DEPENDS ${MVK_ICD_SRC} ${MVK_DST}
            COMMAND ${CMAKE_COMMAND} -E copy ${MVK_ICD_SRC} ${MVK_ICD_DST})
    add_custom_command(
            OUTPUT ${MVK_DYLIB_DST}
            DEPENDS ${MVK_DYLIB_SRC} ${MVK_DST}
            COMMAND ${CMAKE_COMMAND} -E copy ${MVK_DYLIB_SRC} ${MVK_DYLIB_DST})
    add_custom_target(CopyMoltenVK DEPENDS ${MVK_ICD_DST} ${MVK_DYLIB_DST})
    add_dependencies(CopyMoltenVK MoltenVK)
    add_dependencies(LibertyRecomp CopyMoltenVK)
else()
    add_executable(LibertyRecomp ${LIBERTY_RECOMP_CXX_SOURCES})
endif()

if (LIBERTY_RECOMP_FLATPAK)
    target_compile_definitions(LibertyRecomp PRIVATE
        "LIBERTY_RECOMP_FLATPAK"
        "GAME_INSTALL_DIRECTORY=\"/var/data\""
    )
endif()

find_package(CURL REQUIRED)

if (LIBERTY_RECOMP_METAL)
    target_compile_definitions(LibertyRecomp PRIVATE LIBERTY_RECOMP_METAL)
endif()

if (LIBERTY_RECOMP_SWITCH)
    target_compile_definitions(LibertyRecomp PRIVATE
        LIBERTY_RECOMP_SWITCH
        IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
    )
    if (LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT)
        target_compile_definitions(LibertyRecomp PRIVATE
            LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT
            "LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE=${LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE}"
            "LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE=${LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE}")
        if (NOT LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE STREQUAL "")
            target_compile_definitions(LibertyRecomp PRIVATE
                "LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE=${LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE}")
        endif()
    endif()
    if (LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP)
        target_compile_definitions(LibertyRecomp PRIVATE LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP)
    endif()
    if (LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS)
        target_compile_definitions(LibertyRecomp PRIVATE LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS)
    endif()

    find_program(LIBERTY_SWITCH_NACPTOOL nacptool
        HINTS "${DEVKITPRO}/tools/bin"
        REQUIRED)
    find_program(LIBERTY_SWITCH_ELF2NRO elf2nro
        HINTS "${DEVKITPRO}/tools/bin"
        REQUIRED)
    find_program(LIBERTY_SWITCH_ELF2NSO elf2nso
        HINTS "${DEVKITPRO}/tools/bin"
        REQUIRED)
    find_program(LIBERTY_SWITCH_NPDMTOOL npdmtool
        HINTS "${DEVKITPRO}/tools/bin"
        REQUIRED)
    find_program(LIBERTY_SWITCH_BUILD_PFS0 build_pfs0
        HINTS "${DEVKITPRO}/tools/bin"
        REQUIRED)

    set(LIBERTY_SWITCH_NACP "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecomp.nacp")
    set(LIBERTY_SWITCH_NRO "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecomp.nro")
    set(LIBERTY_SWITCH_ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs"
        CACHE PATH "RomFS directory embedded into the Switch audit NRO")
    set(LIBERTY_SWITCH_ICON "${PROJECT_SOURCE_DIR}/../LibertyRecompResources/images/game_icon.png"
        CACHE FILEPATH "Icon embedded into the Switch audit NRO")
    if (CMAKE_EXE_LINKER_FLAGS MATCHES "(^| )-specs=")
        set(LIBERTY_SWITCH_TARGET_SPECS_OPTION "")
    else()
        set(LIBERTY_SWITCH_TARGET_SPECS_OPTION "-specs=${LIBERTY_SWITCH_SPECS}")
    endif()

    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_NACP}"
        COMMAND "${LIBERTY_SWITCH_NACPTOOL}"
            --create
            "${LIBERTY_SWITCH_APP_TITLE}"
            "${LIBERTY_SWITCH_APP_AUTHOR}"
            "${LIBERTY_SWITCH_APP_VERSION}"
            "${LIBERTY_SWITCH_NACP}"
        COMMENT "Generating Switch NACP metadata"
        VERBATIM)

    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_NRO}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBERTY_SWITCH_ROMFS_DIR}"
        COMMAND "${LIBERTY_SWITCH_ELF2NRO}"
            "$<TARGET_FILE:LibertyRecomp>"
            "${LIBERTY_SWITCH_NRO}"
            "--nacp=${LIBERTY_SWITCH_NACP}"
            "--romfsdir=${LIBERTY_SWITCH_ROMFS_DIR}"
            "--icon=${LIBERTY_SWITCH_ICON}"
        DEPENDS LibertyRecomp "${LIBERTY_SWITCH_NACP}" "${LIBERTY_SWITCH_ICON}"
        COMMENT "Packaging Switch audit NRO"
        VERBATIM)

    add_custom_target(LibertyRecompNro ALL
        DEPENDS "${LIBERTY_SWITCH_NRO}")

    set(LIBERTY_SWITCH_EXEFS_NPDM_JSON "${CMAKE_CURRENT_SOURCE_DIR}/os/switch/liberty_recomp_npdm.json")
    set(LIBERTY_SWITCH_EXEFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecomp.exefs")
    set(LIBERTY_SWITCH_NSO "${LIBERTY_SWITCH_EXEFS_DIR}/main")
    set(LIBERTY_SWITCH_NPDM "${LIBERTY_SWITCH_EXEFS_DIR}/main.npdm")
    set(LIBERTY_SWITCH_EXEFS "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompExefs.nsp")
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_NSO}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBERTY_SWITCH_EXEFS_DIR}"
        COMMAND "${LIBERTY_SWITCH_ELF2NSO}"
            "$<TARGET_FILE:LibertyRecomp>"
            "${LIBERTY_SWITCH_NSO}"
        DEPENDS LibertyRecomp
        COMMENT "Converting Switch audit executable to NSO"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_NPDM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBERTY_SWITCH_EXEFS_DIR}"
        COMMAND "${LIBERTY_SWITCH_NPDMTOOL}"
            "${LIBERTY_SWITCH_EXEFS_NPDM_JSON}"
            "${LIBERTY_SWITCH_NPDM}"
        DEPENDS "${LIBERTY_SWITCH_EXEFS_NPDM_JSON}"
        COMMENT "Generating Switch audit NPDM"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_EXEFS}"
        COMMAND "${LIBERTY_SWITCH_BUILD_PFS0}"
            "${LIBERTY_SWITCH_EXEFS_DIR}"
            "${LIBERTY_SWITCH_EXEFS}"
        DEPENDS "${LIBERTY_SWITCH_NSO}" "${LIBERTY_SWITCH_NPDM}"
        COMMENT "Packaging Switch audit ExeFS PFS0"
        VERBATIM)
    add_custom_target(LibertyRecompExeFs
        DEPENDS "${LIBERTY_SWITCH_EXEFS}")

    add_executable(LibertyRecompBootProbe
        "os/switch/boot_probe_switch.c")
    target_link_options(LibertyRecompBootProbe PRIVATE
        ${LIBERTY_SWITCH_TARGET_SPECS_OPTION}
        "-L${DEVKITPRO}/portlibs/switch/lib"
        "-L${DEVKITPRO}/libnx/lib"
        "-L${DEVKITA64}/aarch64-none-elf/lib/pic"
        "-L${DEVKITA64}/aarch64-none-elf/lib"
        "-Wl,--gc-sections")
    target_link_libraries(LibertyRecompBootProbe PRIVATE nx)

    set(LIBERTY_SWITCH_BOOT_PROBE_NACP "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompBootProbe.nacp")
    set(LIBERTY_SWITCH_BOOT_PROBE_NRO "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompBootProbe.nro")
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_BOOT_PROBE_NACP}"
        COMMAND "${LIBERTY_SWITCH_NACPTOOL}"
            --create
            "Liberty Boot Probe"
            "${LIBERTY_SWITCH_APP_AUTHOR}"
            "${LIBERTY_SWITCH_APP_VERSION}"
            "${LIBERTY_SWITCH_BOOT_PROBE_NACP}"
        COMMENT "Generating Switch boot probe NACP metadata"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_BOOT_PROBE_NRO}"
        COMMAND "${LIBERTY_SWITCH_ELF2NRO}"
            "$<TARGET_FILE:LibertyRecompBootProbe>"
            "${LIBERTY_SWITCH_BOOT_PROBE_NRO}"
            "--nacp=${LIBERTY_SWITCH_BOOT_PROBE_NACP}"
            "--icon=${LIBERTY_SWITCH_ICON}"
        DEPENDS LibertyRecompBootProbe "${LIBERTY_SWITCH_BOOT_PROBE_NACP}" "${LIBERTY_SWITCH_ICON}"
        COMMENT "Packaging Switch boot probe NRO"
        VERBATIM)
    add_custom_target(LibertyRecompBootProbeNro
        DEPENDS "${LIBERTY_SWITCH_BOOT_PROBE_NRO}")

    add_executable(LibertyRecompMemoryProbe
        "os/switch/memory_probe_switch.c")
    target_link_options(LibertyRecompMemoryProbe PRIVATE
        ${LIBERTY_SWITCH_TARGET_SPECS_OPTION}
        "-L${DEVKITPRO}/portlibs/switch/lib"
        "-L${DEVKITPRO}/libnx/lib"
        "-L${DEVKITA64}/aarch64-none-elf/lib/pic"
        "-L${DEVKITA64}/aarch64-none-elf/lib"
        "-Wl,--gc-sections")
    target_link_libraries(LibertyRecompMemoryProbe PRIVATE nx)

    set(LIBERTY_SWITCH_MEMORY_PROBE_NACP "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompMemoryProbe.nacp")
    set(LIBERTY_SWITCH_MEMORY_PROBE_NRO "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompMemoryProbe.nro")
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_MEMORY_PROBE_NACP}"
        COMMAND "${LIBERTY_SWITCH_NACPTOOL}"
            --create
            "Liberty Memory Probe"
            "${LIBERTY_SWITCH_APP_AUTHOR}"
            "${LIBERTY_SWITCH_APP_VERSION}"
            "${LIBERTY_SWITCH_MEMORY_PROBE_NACP}"
        COMMENT "Generating Switch memory probe NACP metadata"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_MEMORY_PROBE_NRO}"
        COMMAND "${LIBERTY_SWITCH_ELF2NRO}"
            "$<TARGET_FILE:LibertyRecompMemoryProbe>"
            "${LIBERTY_SWITCH_MEMORY_PROBE_NRO}"
            "--nacp=${LIBERTY_SWITCH_MEMORY_PROBE_NACP}"
            "--icon=${LIBERTY_SWITCH_ICON}"
        DEPENDS LibertyRecompMemoryProbe "${LIBERTY_SWITCH_MEMORY_PROBE_NACP}" "${LIBERTY_SWITCH_ICON}"
        COMMENT "Packaging Switch memory probe NRO"
        VERBATIM)
    add_custom_target(LibertyRecompMemoryProbeNro
        DEPENDS "${LIBERTY_SWITCH_MEMORY_PROBE_NRO}")

    set(LIBERTY_SWITCH_MEMORY_PROBE_NPDM_JSON "${CMAKE_CURRENT_SOURCE_DIR}/os/switch/memory_probe_npdm.json")
    set(LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompMemoryProbe.exefs")
    set(LIBERTY_SWITCH_MEMORY_PROBE_NSO "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR}/main")
    set(LIBERTY_SWITCH_MEMORY_PROBE_NPDM "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR}/main.npdm")
    set(LIBERTY_SWITCH_MEMORY_PROBE_EXEFS "${CMAKE_CURRENT_BINARY_DIR}/LibertyRecompMemoryProbeExefs.nsp")
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_MEMORY_PROBE_NSO}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR}"
        COMMAND "${LIBERTY_SWITCH_ELF2NSO}"
            "$<TARGET_FILE:LibertyRecompMemoryProbe>"
            "${LIBERTY_SWITCH_MEMORY_PROBE_NSO}"
        DEPENDS LibertyRecompMemoryProbe
        COMMENT "Converting Switch memory probe to NSO"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_MEMORY_PROBE_NPDM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR}"
        COMMAND "${LIBERTY_SWITCH_NPDMTOOL}"
            "${LIBERTY_SWITCH_MEMORY_PROBE_NPDM_JSON}"
            "${LIBERTY_SWITCH_MEMORY_PROBE_NPDM}"
        DEPENDS "${LIBERTY_SWITCH_MEMORY_PROBE_NPDM_JSON}"
        COMMENT "Generating Switch memory probe NPDM"
        VERBATIM)
    add_custom_command(
        OUTPUT "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS}"
        COMMAND "${LIBERTY_SWITCH_BUILD_PFS0}"
            "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS_DIR}"
            "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS}"
        DEPENDS "${LIBERTY_SWITCH_MEMORY_PROBE_NSO}" "${LIBERTY_SWITCH_MEMORY_PROBE_NPDM}"
        COMMENT "Packaging Switch memory probe ExeFS PFS0"
        VERBATIM)
    add_custom_target(LibertyRecompMemoryProbeExeFs
        DEPENDS "${LIBERTY_SWITCH_MEMORY_PROBE_EXEFS}")
endif()

if (LIBERTY_RECOMP_D3D12)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
    target_compile_definitions(LibertyRecomp PRIVATE LIBERTY_RECOMP_D3D12)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/D3D12)
    add_custom_command(TARGET LibertyRecomp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:LibertyRecomp>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:LibertyRecomp>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectXShaderCompiler,IMPORTED_LOCATION> $<TARGET_FILE_DIR:LibertyRecomp>
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DXIL,IMPORTED_LOCATION> $<TARGET_FILE_DIR:LibertyRecomp>
            COMMAND_EXPAND_LISTS
    )

    target_link_libraries(LibertyRecomp PRIVATE
            Microsoft::DirectXShaderCompiler
            Microsoft::DXIL
            dxgi
    )
endif()

if (WIN32)
    target_link_libraries(LibertyRecomp PRIVATE
        comctl32
        dwmapi
        ntdll
        Shcore
        Synchronization
        winmm
        windowsapp
    )
    target_link_options(LibertyRecomp PRIVATE /FORCE:MULTIPLE)
else()
    target_link_libraries(LibertyRecomp PRIVATE fmt::fmt)
endif()

target_link_libraries(LibertyRecomp PRIVATE
    libzstd_static
    o1heap
    SDL2_mixer
    tomlplusplus::tomlplusplus
    LibertyRecompLib
    xxHash::xxhash
    CURL::libcurl
    plume
)

if (NOT LIBERTY_RECOMP_SWITCH)
    target_link_libraries(LibertyRecomp PRIVATE XenonUtils)
endif()

if (LIBERTY_RECOMP_SWITCH)
    target_link_options(LibertyRecomp PRIVATE
        ${LIBERTY_SWITCH_TARGET_SPECS_OPTION}
        "-L${DEVKITPRO}/portlibs/switch/lib"
        "-L${DEVKITPRO}/libnx/lib"
        "-L${DEVKITA64}/aarch64-none-elf/lib/pic"
        "-L${DEVKITA64}/aarch64-none-elf/lib"
        "-Wl,--gc-sections")
    target_link_directories(LibertyRecomp PRIVATE
        "${DEVKITPRO}/portlibs/switch/lib"
        "${DEVKITPRO}/libnx/lib"
        "${DEVKITA64}/aarch64-none-elf/lib/pic"
        "${DEVKITA64}/aarch64-none-elf/lib")
    target_link_libraries(LibertyRecomp PRIVATE
        "${DEVKITPRO}/portlibs/switch/lib/libSDL2.a"
        EGL
        stdc++
        glapi
        drm_nouveau
        nx
        pthread)
else()
    target_link_libraries(LibertyRecomp PRIVATE SDL2::SDL2-static)
endif()

if (NOT LIBERTY_RECOMP_SWITCH)
    target_link_libraries(LibertyRecomp PRIVATE msdf-atlas-gen::msdf-atlas-gen)
    target_link_libraries(LibertyRecomp PRIVATE nfd::nfd)
endif()

if (NOT LIBERTY_RECOMP_SWITCH)
    target_link_libraries(LibertyRecomp PRIVATE ffmpeg)
    target_link_libraries(LibertyRecomp PRIVATE GameNetworkingSockets::static)
endif()

if (WIN32)
    target_link_libraries(LibertyRecomp PRIVATE "$<TARGET_FILE:LibertyRecompLib>")
endif()

# Link abseil libraries for GameNetworkingSockets (protobuf dependency)
if (APPLE)
    find_library(ABSL_LOG_INTERNAL_CHECK_OP absl_log_internal_check_op HINTS /opt/homebrew/lib)
    find_library(ABSL_LOG_INTERNAL_MESSAGE absl_log_internal_message HINTS /opt/homebrew/lib)
    find_library(ABSL_LOG_INTERNAL_NULLGUARD absl_log_internal_nullguard HINTS /opt/homebrew/lib)
    find_library(ABSL_EXAMINE_STACK absl_examine_stack HINTS /opt/homebrew/lib)
    find_library(ABSL_STRERROR absl_strerror HINTS /opt/homebrew/lib)
    find_library(ABSL_LOG_INTERNAL_GLOBALS absl_log_internal_globals HINTS /opt/homebrew/lib)
    find_library(ABSL_RAW_HASH_SET absl_raw_hash_set HINTS /opt/homebrew/lib)
    find_library(ABSL_HASHTABLEZ_SAMPLER absl_hashtablez_sampler HINTS /opt/homebrew/lib)
    find_library(ABSL_LOG_SINK absl_log_sink HINTS /opt/homebrew/lib)
    if (ABSL_LOG_INTERNAL_CHECK_OP)
        target_link_libraries(LibertyRecomp PRIVATE
            ${ABSL_LOG_INTERNAL_CHECK_OP}
            ${ABSL_LOG_INTERNAL_MESSAGE}
            ${ABSL_LOG_INTERNAL_NULLGUARD}
            ${ABSL_EXAMINE_STACK}
            ${ABSL_STRERROR}
            ${ABSL_LOG_INTERNAL_GLOBALS}
            ${ABSL_RAW_HASH_SET}
            ${ABSL_HASHTABLEZ_SAMPLER}
            ${ABSL_LOG_SINK}
        )
    endif()
endif()

target_include_directories(LibertyRecomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
    ${LIBERTY_RECOMP_THIRDPARTY_INCLUDES}
    "${CMAKE_SOURCE_DIR}/glue/rexglue-sdk-main/thirdparty/spdlog/include"
    "${CMAKE_SOURCE_DIR}/glue/rexglue-sdk-main/thirdparty/fmt/include"
    "${CMAKE_SOURCE_DIR}/glue/rexglue-sdk-main/thirdparty/simde"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty/tiny-AES-c"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty/libmspack/libmspack/mspack"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/thirdparty/TinySHA1"
    "${LIBERTY_RECOMP_TOOLS_ROOT}/XenonRecomp/XenonUtils"
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(LibertyRecomp PRIVATE __PRFCHWINTRIN_H)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    find_package(X11 REQUIRED)
    pkg_search_module(GLIB REQUIRED glib-2.0)
    pkg_search_module(GIO REQUIRED gio-2.0)
    target_include_directories(LibertyRecomp PRIVATE ${X11_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
    target_link_libraries(LibertyRecomp PRIVATE ${X11_LIBRARIES} ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
endif()

target_precompile_headers(LibertyRecomp PUBLIC ${LIBERTY_RECOMP_PRECOMPILED_HEADERS})

function(compile_shader FILE_PATH TARGET_NAME)
    set(HLSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gpu/shader/hlsl/${FILE_PATH}.hlsl)
    set(MSL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gpu/shader/msl/${FILE_PATH}.metal)
    cmake_path(GET HLSL_FILE_PATH STEM HLSL_NAME)
    cmake_path(GET MSL_FILE_PATH STEM MSL_NAME)
    if (LIBERTY_RECOMP_METAL)
        add_custom_command(
                OUTPUT ${MSL_FILE_PATH}.ir
                COMMAND ${XCRUN_TOOL} -sdk macosx metal -o ${MSL_FILE_PATH}.ir -c ${MSL_FILE_PATH} -D__air__ -frecord-sources -gline-tables-only
                DEPENDS ${MSL_FILE_PATH}
        )
        add_custom_command(
                OUTPUT ${MSL_FILE_PATH}.metallib
                COMMAND ${XCRUN_TOOL} -sdk macosx metallib -o ${MSL_FILE_PATH}.metallib ${MSL_FILE_PATH}.ir
                DEPENDS ${MSL_FILE_PATH}.ir
        )
        BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${MSL_FILE_PATH}.metallib" DEST_FILE "${MSL_FILE_PATH}.metallib" ARRAY_NAME "g_${MSL_NAME}_air")
    endif()
    if (LIBERTY_RECOMP_D3D12)
        add_custom_command(
            OUTPUT ${HLSL_FILE_PATH}.dxil.h
            COMMAND ${DIRECTX_DXC_TOOL} -T ${TARGET_NAME} -HV 2021 -all-resources-bound -Wno-ignored-attributes -E shaderMain -Fh ${HLSL_FILE_PATH}.dxil.h ${HLSL_FILE_PATH} -Vn g_${HLSL_NAME}_dxil
            DEPENDS ${HLSL_FILE_PATH}
        )
        target_sources(LibertyRecomp PRIVATE ${HLSL_FILE_PATH}.dxil.h)
    endif()
    add_custom_command(
        OUTPUT ${HLSL_FILE_PATH}.spirv.h
        COMMAND ${DIRECTX_DXC_TOOL} -T ${TARGET_NAME} -HV 2021 -all-resources-bound -spirv -fvk-use-dx-layout ${ARGN} -E shaderMain -Fh ${HLSL_FILE_PATH}.spirv.h ${HLSL_FILE_PATH} -Vn g_${HLSL_NAME}_spirv
        DEPENDS ${HLSL_FILE_PATH}
    )
    target_sources(LibertyRecomp PRIVATE ${HLSL_FILE_PATH}.spirv.h)
endfunction()

function(compile_vertex_shader FILE_PATH)
    compile_shader(${FILE_PATH} vs_6_0 -fvk-invert-y -DLIBERTY_RECOMP)
endfunction()

function(compile_pixel_shader FILE_PATH)
    compile_shader(${FILE_PATH} ps_6_0 -DLIBERTY_RECOMP)
endfunction()

compile_pixel_shader(blend_color_alpha_ps)
compile_vertex_shader(copy_vs)
compile_pixel_shader(copy_color_ps)
compile_pixel_shader(copy_depth_ps)
compile_pixel_shader(csd_filter_ps)
compile_vertex_shader(csd_no_tex_vs)
compile_vertex_shader(csd_vs)
compile_vertex_shader(enhanced_burnout_blur_vs)
compile_pixel_shader(enhanced_burnout_blur_ps)
compile_pixel_shader(gaussian_blur_3x3)
compile_pixel_shader(gaussian_blur_5x5)
compile_pixel_shader(gaussian_blur_7x7)
compile_pixel_shader(gaussian_blur_9x9)
compile_pixel_shader(gamma_correction_ps)
compile_pixel_shader(imgui_ps)
compile_vertex_shader(imgui_vs)
compile_pixel_shader(resolve_msaa_color_2x)
compile_pixel_shader(resolve_msaa_color_4x)
compile_pixel_shader(resolve_msaa_color_8x)
compile_pixel_shader(resolve_msaa_depth_2x)
compile_pixel_shader(resolve_msaa_depth_4x)
compile_pixel_shader(resolve_msaa_depth_8x)

function(generate_aggregate_header INPUT_DIRECTORY OUTPUT_FILE)
    get_filename_component(ABS_OUTPUT_FILE "${OUTPUT_FILE}" ABSOLUTE)
    file(GLOB_RECURSE HEADER_FILES "${INPUT_DIRECTORY}/*.h")
    set(HEADER_CONTENT "#pragma once\n\n")

    foreach(HEADER_FILE IN LISTS HEADER_FILES)
        get_filename_component(ABS_HEADER_FILE "${HEADER_FILE}" ABSOLUTE)
        if (ABS_HEADER_FILE STREQUAL ABS_OUTPUT_FILE)
            continue()
        endif()
        file(RELATIVE_PATH RELATIVE_HEADER_FILE "${INPUT_DIRECTORY}" "${HEADER_FILE}")
        string(APPEND HEADER_CONTENT "#include \"${RELATIVE_HEADER_FILE}\"\n")
    endforeach()

    if (EXISTS "${OUTPUT_FILE}")
        file(READ "${OUTPUT_FILE}" EXISTING_CONTENT)
        if (EXISTING_CONTENT STREQUAL HEADER_CONTENT)
            return()
        endif()
    endif()

    file(WRITE "${OUTPUT_FILE}" "${HEADER_CONTENT}")
endfunction()

generate_aggregate_header(
    "${CMAKE_CURRENT_SOURCE_DIR}/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/api/Liberty.h"
)

set(RESOURCES_SOURCE_PATH "${PROJECT_SOURCE_DIR}/../LibertyRecompResources")
set(RESOURCES_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/res")

## Miscellaneous ##
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/bc_diff/button_bc_diff.bin" DEST_FILE "${RESOURCES_OUTPUT_PATH}/bc_diff/button_bc_diff.bin" ARRAY_NAME "g_button_bc_diff" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/font/im_font_atlas.bin" DEST_FILE "${RESOURCES_OUTPUT_PATH}/font/im_font_atlas.bin" ARRAY_NAME "g_im_font_atlas" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/font/im_font_atlas.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/font/im_font_atlas.dds" ARRAY_NAME "g_im_font_atlas_texture" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/achievements_menu/trophy.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/achievements_menu/trophy.dds" ARRAY_NAME "g_trophy" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/button_window.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/button_window.dds" ARRAY_NAME "g_button_window" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/controller.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/controller.dds" ARRAY_NAME "g_controller" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/libertyrecomp.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/libertyrecomp.dds" ARRAY_NAME "g_libertyrecomp" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/kbm.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/kbm.dds" ARRAY_NAME "g_kbm" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/main_menu1.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/main_menu1.dds" ARRAY_NAME "g_main_menu1" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/main_menu7.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/main_menu7.dds" ARRAY_NAME "g_main_menu7" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/main_menu8.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/main_menu8.dds" ARRAY_NAME "g_main_menu8" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/main_menu9.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/main_menu9.dds" ARRAY_NAME "g_main_menu9" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/arrow.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/arrow.dds" ARRAY_NAME "g_arrow" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/window.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/window.dds" ARRAY_NAME "g_window" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/common/select_arrow.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/common/select_arrow.dds" ARRAY_NAME "g_select_arrow" COMPRESSION_TYPE "zstd")

## Installer ##
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/gta4_logo.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/gta4_logo.dds" ARRAY_NAME "g_gta4_logo" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/DLC/tlad_logo.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/DLC/tlad_logo.dds" ARRAY_NAME "g_tlad_logo" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/DLC/tbogt_logo.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/DLC/tbogt_logo.dds" ARRAY_NAME "g_tbogt_logo" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_001.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_001.dds" ARRAY_NAME "g_install_001" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_002.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_002.dds" ARRAY_NAME "g_install_002" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_003.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_003.dds" ARRAY_NAME "g_install_003" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_004.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_004.dds" ARRAY_NAME "g_install_004" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_005.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_005.dds" ARRAY_NAME "g_install_005" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_006.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_006.dds" ARRAY_NAME "g_install_006" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_007.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_007.dds" ARRAY_NAME "g_install_007" COMPRESSION_TYPE "zstd")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/installer/install_008.dds" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/installer/install_008.dds" ARRAY_NAME "g_install_008" COMPRESSION_TYPE "zstd")

## Game Icon ##
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/images/game_icon.bmp" DEST_FILE "${RESOURCES_OUTPUT_PATH}/images/game_icon.bmp" ARRAY_NAME "g_game_icon")

## Audio ##
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/music/installer.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/music/installer.ogg" ARRAY_NAME "g_installer_music")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/window_open.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/window_open.ogg" ARRAY_NAME "g_window_open")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/window_close.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/window_close.ogg" ARRAY_NAME "g_window_close")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/cursor2.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/cursor2.ogg" ARRAY_NAME "g_cursor2")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/deside.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/deside.ogg" ARRAY_NAME "g_deside")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/move.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/move.ogg" ARRAY_NAME "g_move")
BIN2C(TARGET_OBJ LibertyRecomp SOURCE_FILE "${RESOURCES_SOURCE_PATH}/sounds/main_deside.ogg" DEST_FILE "${RESOURCES_OUTPUT_PATH}/sounds/main_deside.ogg" ARRAY_NAME "g_main_deside")
