# GTA4 Static Recompilation Switch Port Notes

## Project Goal

Build a GTA IV / LibertyRecomp static recompilation project and audit it toward a Nintendo Switch port.

This is not a playable Switch port yet. The current goal is to make the codebase configure and compile under devkitPro/devkitA64, expose missing platform layers, and keep a clear map of what still needs real reverse engineering or platform work.

## Main Paths

- Project/game files: `D:\GTA4 NS`
- Main git repo: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp`
- Current Codex workspace: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns`
- Switch audit build: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug`
- devkitPro: `C:\devkitPro`
- devkitA64: `C:\devkitPro\devkitA64`
- Session summary log: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\SWITCH_PORT_AUDIT_LOG_2026-06-14.md`
- Switch SD layout draft: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\SWITCH_SD_LAYOUT.md`
- Local Nintendo SDK/reference notes: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\NINTENDO_SDK_USAGE_NOTES_2026-06-14.md`

## Current Status

- devkitPro, devkitA64, libnx, switch-sdl2, switch-sdl2_mixer, and switch-curl are installed and usable.
- ReXGlue codegen has been built and used to generate GTA IV recompilation sources.
- Windows host build has produced `LibertyRecomp.exe`, but it is only scaffolding and not playable.
- Switch CMake configure succeeds.
- `LibertyRecompLib` builds and links as a Switch static library.
- Switch main program now compiles and links as a devkitA64 ELF:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecomp`
- Switch build now also emits a homebrew NRO audit artifact:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecomp.nro`
- Switch build can also emit a minimal independent boot probe NRO:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecompBootProbe.nro`
- Switch build can also emit an independent guest-memory SVC probe NRO:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecompMemoryProbe.nro`
- Switch build can also emit an independent guest-memory ExeFS/NPDM probe package:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecompMemoryProbeExefs.nsp`
- Switch build can also emit a full LibertyRecomp ExeFS/NPDM audit package:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecompExefs.nsp`
- A separate guest-memory audit build directory is now available:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-guest-memory-audit-debug`
  It enables `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT` and `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP` for ExeFS/NPDM testing only.
- This NRO is not playable. It is only an ELF -> NRO packaging/audit milestone with empty placeholder RomFS and many runtime stubs still active.
- The latest successful Ninja run includes basic `romfs:`/`sdmc:` startup, `sdmc:` already-mounted detection, Switch pre-main breadcrumbs, SD logging, a visible missing-content diagnostic path, and a guard that stops before host/guest startup while Switch guest memory is disabled.
- Latest confirmed default full NRO/ExeFS Build ID: `4a5ab140d5a54352bcf968b26ea84fc4e20e64c0`.
- Latest default full ExeFS/NRO rebuild after the install-check audit changes: Build ID `7c5ed727f14026550070339b484eebf66494a12d`; build, `readelf -dW`, and a Ryujinx missing-content smoke test were verified. This was not a gameplay test.
- Latest Switch config-audit ExeFS Build ID: `e9ec24d9e526020fd8f8b1d392b7f60a69bcaeb9`. It verified that an old duplicate-table `config.toml` is rewritten without entering the TOML exception path, then a second run parses the rewritten config and returns from `Config::Load()`.
- Latest Switch install-check audit ExeFS Build ID: `41fe5790406b362571818cc479cdc10337905172`. It verified that `Config::Load()` returns, `Installer::checkGameInstall()` checks `sdmc:/switch/LibertyRecomp/game/default.xex`, and both missing-file and temporary-present-file paths stop before host startup, module loading, or guest code.
- Latest confirmed guest-memory audit ExeFS Build ID: `04e2f6197b65eb00c204af28d1d6e791718e779d`.
- Latest narrow guest-memory/function-table audit ExeFS Build ID: `df2f6f7d433534156c4b0f6a19017e55d5302406`. This uses `4 KiB` low memory, `4 KiB` physical heap, full scanned image/function-table mapping, does not skip function mappings, and stops in `main()` after successful mapping/function-table initialization. It is an audit variant only.
- Latest retained-function-table capacity audit ExeFS Build ID: `14293a41cb382b5346b04a31005477554db1cb56`. This uses `4 KiB` low memory, `15 MiB` physical heap, full scanned image/function-table mapping (`0x24A0000` bytes), does not skip function mappings, and stops in `main()` after successful mapping/function-table initialization. It is an audit variant only.
- Latest startup-memory/Xenon-init audit ExeFS Build ID: `539c9829646e4f6e847b882ee8468d40cc579702`. This uses `0x30000` low memory, `15 MiB` physical heap, maps the Xenon fixed memory span `0x82000000..0x831F0000`, skips generated function-table insertion, bypasses host config/content/module loading, reaches `KiSystemStartup()` heap/Xenon initialization, then holds on a visible diagnostic. It is an audit variant only.
- Latest visible boot probe Build ID: `d32811733bd2fe5c2e8172085c607fb1379c2903`.
- Latest memory probe Build ID: `a528c82fb5a99ebcb4b794ad16cde8defac33fc5`.
- Hardware retest of Build ID `5c7d712fb408d740c9c22f0ca6696fe65a96287b` cleanly returned to the Homebrew/Menu page instead of system-crashing. The user-provided software report `C:\Users\Jellybone\Desktop\ed9d2c85-a785-9c48-5f81-92a5be07987a` has `ErrorCode=2128-0051`, `AbortFlag=true`, `ApplicationAbortFlag=true`, and `CreateProcessFailureFlag=false`, with no PC/LR fatal context.
- Hardware retest of Build ID `496fa2f332624750b943c4d60d978ac343b138c9` still black-screened then returned without creating `sdmc:/switch/LibertyRecomp/LibertyRecomp.log`. The report `C:\Users\Jellybone\AppData\Local\Temp\03034866-2b22-2649-6ffe-b06c5a6f96e0` again shows application abort (`ErrorCode=2128-0051`, `AbortFlag=true`, `ApplicationAbortFlag=true`, `CreateProcessFailureFlag=false`) and no PC/LR fatal context.
- Hardware retest of Build ID `afa9072da6ad53ee46d05bfc7a9aadf901e8eff4` still produced no SD log and behaved like the prior software-abort cases. The report `C:\Users\Jellybone\AppData\Local\Temp\02af45f2-d1c3-8b42-4db2-a180f0d13283` has `ErrorCode=2128-0051`, `AbortFlag=true`, `ApplicationAbortFlag=true`, `CreateProcessFailureFlag=false`, `ApplicationAliveTime=0`, and no PC/LR fatal context.
- Hardware retest of the first minimal boot probe Build ID `5208ed125bee8ea041514c749fef43d4573bd86a` held on hardware but did not create `sdmc:/switch/LibertyRecomp/boot_probe.log`.
- Hardware test of visible boot probe Build ID `d4f9713bc86bb148ef9694404d9219b57c0d63d2` showed `fsdevMountSdmc: 0x00000559`, `SD mount failed; file write skipped`, and held on the diagnostic screen. Ryujinx showed the same `0x559`, but the corrected probe proved `sdmc:` was already registered and file writes work when the existing device is used.
- Ryujinx smoke test of full NRO Build ID `bf943e8125cc1124b5bdd52cd152efed05c83cd6` reached `main()`, mounted `romfs:` successfully, confirmed `sdmc:` usable, wrote `sdmc:/switch/LibertyRecomp/LibertyRecomp.log`, logged the missing `sdmc:/switch/LibertyRecomp/game/default.xex`, and held on the visible missing-content diagnostic path. This is still not playable.
- A second Ryujinx smoke test with a temporary dummy `sdmc:/switch/LibertyRecomp/game/default.xex` confirmed the current build logs `Guest memory disabled` and stops before host startup / `KiSystemStartup()`. The dummy file was removed after the test.
- Ryujinx smoke testing of `LibertyRecompMemoryProbe.nro` confirmed that current NRO/hbloader launch has `SystemResourceSizeTotal=0` and `SystemResourceSizeUsed=0`; `svcMapPhysicalMemory()` returns `0x0000FA01` for small ASLR ranges, 4 GiB reservation windows, and fixed heap-region candidates even after reducing `__nx_heap_size` to 96 MiB. Treat the current sparse `svcMapPhysicalMemory()` guest-memory backend as blocked in NRO form unless packaging can provide NPDM system resource budget.
- Ryujinx smoke testing of `LibertyRecompMemoryProbeExefs.nsp` confirmed the NPDM `system_resource_size=0x08000000` path works: `SystemResourceSizeTotal=0x08000000`, `SystemResourceSizeUsed=0x1000`. `svcMapPhysicalMemory()` still returns `0x0000DC01` for ASLR/heap addresses, but succeeds for Alias region addresses such as `0x80000000` and `0x102000000`.
- Ryujinx smoke testing of `LibertyRecompExefs.nsp` confirmed the full app can launch as an ExeFS/NPDM audit package and reach the same missing-content preflight as the NRO. Because the devkitPro ExeFS PFS0 has no RomFS data storage, Switch startup currently skips `romfsMountSelf()` when `envIsNso()` reports true and relies on SD content paths.
- Ryujinx smoke testing of the separate guest-memory audit ExeFS confirmed the full process can map Alias-region sparse guest-memory windows of `32 MiB` low memory, `64 KiB` XMA I/O, `32 MiB` physical heap, and `32 MiB` image/function-table audit space under NPDM `system_resource_size=0x08000000`, then reach `main()` and stop before content preflight or host startup. A full planned mapping attempt with a `64 MiB` low-memory window failed immediately with kernel `0x0000D001` (`OutOfMemory`).
- Follow-up guest-memory audit work split `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP` from function-mapping initialization. The full app NPDM `system_resource_size` is now `0x10000000`. With function mappings retained and full scanned image/function-table mapping enabled, a `32 MiB` low + `32 MiB` physical heap attempt still fails at the physical heap map with `0x0000D001`, but a narrow `4 KiB` low + `4 KiB` physical heap audit reaches `Memory::Memory()` completion and the `main()` audit stop.
- Follow-up capacity auditing added per-map SystemResource logging and moved guest-memory audit size defines to `kernel/memory.cpp` only, reducing size-tuning rebuilds to `memory.cpp` + link/package. Ryujinx kept `SystemResourceSizeUsed=0x1000` through both successful and failed `svcMapPhysicalMemory()` runs, so that counter is not useful for this failure mode. With `4 KiB` low memory and the full scanned image/function-table mapping retained, `15 MiB` physical heap succeeds and `16 MiB` physical heap fails. The CMake audit defaults are now `low=0x1000`, `physical=0xF00000`.
- Default ExeFS/NRO was revalidated after the mapping-budget audit changes. Ryujinx log `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_11-04-06.log` confirmed the default package still keeps guest memory disabled, reaches `main()`, skips RomFS in ExeFS/NSO mode, appends the SD log, and stops at missing `sdmc:/switch/LibertyRecomp/game/default.xex`.
- Earlier `-Wl,-z,notext` debt was removed by linking Switch builds against devkitA64 PIC libraries first, preserving libnx `-z text`, generating build-local Switch specs/linker script files, adding `-z gcs=never`, packaging `LibertyRecompNro`, and validating the RELR targets no longer touch read-only LOAD segments.

## Important Switch Port Reality

Do not claim the game is playable.

Current Switch work is a compile/audit pass. Several systems are temporary stubs:

- Graphics: Plume/Vulkan is stubbed for Switch. A real Switch backend still needs deko3d or another Switch-specific renderer.
- Audio: XMA/FFmpeg decoding is stubbed via `apu/xma_decoder_switch_stub.cpp`; real game audio will not work until XMA is ported or replaced.
- Installer/UI desktop features: native file dialogs, browser launching, and desktop installer flows are skipped or stubbed on Switch.
- Networking: GameNetworkingSockets/P2P is disabled by a Switch audit stub. Real online/session support is not present.
- File mapping: Switch currently uses a read-into-memory fallback instead of desktop `mmap`.
- Guest memory/page protection: current Switch sparse backing store is for compile/runtime audit only; runtime memory layout, on-demand page mapping, and protection need real design.

## Recent Technical Work

- Added Switch toolchain support in `toolchains/switch-libnx.cmake`.
- Adjusted CMake to bypass vcpkg on Switch and use devkitPro portlibs.
- Skipped non-Switch dependencies such as NFD, FFmpeg, GameNetworkingSockets, and desktop Plume builds.
- Added Switch installer and UI stubs.
- Added ReXGlue platform detection for `__SWITCH__`.
- Fixed GCC/devkitA64 issues:
  - `timegm` replacement for ImPlot.
  - anonymous struct issues in XenonUtils headers.
  - `mmap`/`mprotect` usage.
  - empty variadic logging macros.
  - RTTI disabled while `dynamic_cast` is used.
  - SDL hints missing in Switch SDL2 headers.
  - Switch SDL window handle branch in `game_window.cpp`.
- Fixed Switch link blockers:
  - Added Switch OS source branch in `LibertyRecomp/CMakeLists.txt`.
  - Added Switch no-op registry include.
  - Added Switch platform path handling under `sdmc:/switch/LibertyRecomp`.
  - Added minimal XenonUtils sources for image/XDBF parsing on Switch.
  - Fixed `__imp__XamUserGetSigninState` C linkage.
  - Added Switch audit ReXGlue runtime/CRT symbol stubs.
  - Added Switch audit GameNetworkingSockets stubs.
  - Removed the temporary `-Wl,-z,notext` bypass after switching the Switch link to prefer devkitA64 PIC libraries.
  - Verified final `readelf -dW` no longer reports `TEXTREL`.
  - Verified final RELR targets are all in the writable LOAD: `LOAD0 R E: 0`, `LOAD1 R: 0`, `LOAD2 RW: 63,667`, `outside: 0`.
- Added Switch NRO packaging:
  - CMake now generates a local `liberty-switch.specs` with absolute devkitPro paths and a build-local copy of `libnx/switch.ld` so Ninja does not depend on `/opt/devkitpro` or a process-level `DEVKITPRO`.
  - `LibertyRecompNro` runs `nacptool` and `elf2nro`.
  - `LibertyRecompNro` embeds `LibertyRecompResources/images/game_icon.png` as the NRO icon.
  - Current RomFS is an empty build-directory placeholder; final RomFS/SD layout is still unresolved.
- Replaced the `version_switch.cpp` audit stub with a libnx `hosversionGet()` query using C ABI linkage.
- Added basic Switch RomFS mount lifecycle with `romfsMountSelf("romfs")`/`romfsUnmount("romfs")`; final RomFS contents and error behavior remain unresolved.
- Added basic Switch SD mount lifecycle with existing-device detection around `fsdevMountSdmc()` / `fsdevUnmountDevice("sdmc")`. A `0x00000559` result is not treated as final failure if `sdmc:` is already registered and usable.
- Current Switch startup test build attempts to create `sdmc:/switch/LibertyRecomp`, writes `sdmc:/switch/LibertyRecomp/LibertyRecomp.log`, unmounts SD/RomFS, and then sleeps indefinitely before configuration, installer, content checks, or guest startup.
- The visible libnx framebuffer diagnostic screen remains available in `os/switch/audit_diagnostics_switch.cpp`, but the current final test build intentionally avoids it to isolate SD/main-entry behavior. The helper no longer auto-exits.
- Switch installer stubs are now more honest: `Installer::checkGameInstall()` checks for the module path with `fopen()`, and `InstallerWizard::Run()` reports unimplemented and returns false.
- Replaced the previous Switch `new (std::nothrow) uint8_t[0x100000000]` guest memory allocation with a sparse libnx mapping audit backend, then disabled that backend by default for startup-container builds:
  - `kernel/memory.cpp` uses `svcGetInfo()` to find the process Alias region and maps selected pages with `svcMapPhysicalMemory()`.
  - Ryujinx returned kernel `0x0000FA01` (`InvalidState`) on the first low-heap `svcMapPhysicalMemory()` call. The independent memory probe then showed ordinary memory is not the immediate limiter (`UsedMemorySize=0x06143000` with a 96 MiB libnx heap), but `SystemResourceSizeTotal=0`, so every `svcMapPhysicalMemory()` variant still fails with `0x0000FA01`.
  - The ExeFS/NPDM memory probe showed that with `system_resource_size=0x08000000`, the failure changes from `0x0000FA01` to address validation: ASLR/heap mappings return `0x0000DC01`, while Alias region mappings succeed and are writable.
  - Current default Switch startup builds leave `Memory::base=nullptr` and skip guest-memory mapping unless `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT` is defined. The full app libnx heap is capped to 16 MiB; the independent memory probe still uses a 96 MiB heap for SVC experiments.
  - `kernel/heap.cpp` caps Switch `o1heap` arenas to the currently mapped audit windows for the sparse-memory experiment.
  - This is not a final guest memory implementation; guest startup remains blocked until a real page-backed memory design exists.
- Changed Switch `user/paths.cpp` so pre-main global initialization no longer calls `std::filesystem::exists()` on `romfs:` before RomFS is mounted. This fixed a pre-main exit in `_GLOBAL__sub_I__Z16g_executableRootB5cxx11`.
- Audited `D:\Nintendo` as a local official Nintendo SDK/reference tree and recorded safe usage guidance in `NINTENDO_SDK_USAGE_NOTES_2026-06-14.md`. Use it as local reference for subsystem behavior; do not copy or directly link official SDK headers/libraries into the current libnx NRO target.

## Build Commands

Configure:

```powershell
$build="C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug"
Remove-Item Env:VCPKG_ROOT -ErrorAction SilentlyContinue
$env:DEVKITPRO="C:/devkitPro"
$env:DEVKITA64="C:/devkitPro/devkitA64"
$cmake="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S "C:\Users\Jellybone\Documents\GitHub\LibertyRecomp" -B $build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDEVKITPRO=C:/devkitPro -DDEVKITA64=C:/devkitPro/devkitA64 -DCMAKE_TOOLCHAIN_FILE="C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\toolchains\switch-libnx.cmake" -DCMAKE_MAKE_PROGRAM="C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\bin\ninja.exe"
```

Build:

```powershell
& "C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\bin\ninja.exe" -C "C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug" -j 2
```

The generated recompilation C++ files are large. Switch builds can take a long time, and interrupted Ninja runs may print `premature end of file; recovering` and conservatively rebuild many objects.

## How To Continue

At the start of a new session:

1. Read this file first.
2. Check `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\SWITCH_STUBS.md` before replacing runtime/backend behavior; it tracks temporary Switch audit stubs that must be removed or implemented for a playable port.
3. Check `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\SWITCH_SD_LAYOUT.md` before changing content paths.
4. Check `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\NINTENDO_SDK_USAGE_NOTES_2026-06-14.md` before using anything under `D:\Nintendo`.
5. Check whether any Ninja/devkitA64 processes are already running.
6. Continue the Switch build/package audit.
7. Fix the next real compile, link, package, or runtime-stub blocker with the smallest reasonable platform-specific change.
8. Keep temporary stubs clearly labeled and update `SWITCH_STUBS.md` when adding or removing one.
9. Do not revert unrelated user or generated changes in the dirty worktree.

Expected next phase:

- Improve the current Switch homebrew packaging flow:
  - Refine final NACP metadata if needed.
  - Define the final RomFS/SD layout and harden runtime mount/error behavior.
- Keep both the produced ELF and NRO strictly as compile/audit artifacts until graphics, audio, guest memory, and runtime stubs are replaced.
- Continue runtime crash audit from the new baseline:
  - Latest default full NRO for optional hardware retest: `LibertyRecomp.nro`, Build ID `7c5ed727f14026550070339b484eebf66494a12d`.
  - Expected full-NRO behavior without game content: visible `Missing game content` diagnostic, no gameplay UI, no automatic return, and `sdmc:/switch/LibertyRecomp/LibertyRecomp.log` containing `Switch audit package startup; continuing to content preflight.` plus the missing `sdmc:/switch/LibertyRecomp/game/default.xex` path. Close manually from HOME.
  - Latest visible independent boot probe: `LibertyRecompBootProbe.nro`, Build ID `d32811733bd2fe5c2e8172085c607fb1379c2903`.
  - If the full NRO matches Ryujinx on hardware too, the next practical step is to formalize the SD content layout and continue reworking guest memory before any guest startup attempt.
  - Ryujinx Canary 1.3.269 is now useful for SD-log/main-entry/content-preflight smoke checks after existing `sdmc:` detection, but hardware remains the final check for hold behavior and Sphaira/front-end differences.
- Start replacing stubs tracked in `SWITCH_STUBS.md` with real Switch runtime systems, starting with graphics backend, guest memory/page protection, filesystem/VFS, and audio.
- For guest memory, the non-NRO ExeFS/NPDM path has proven `SystemResourceSize` and Alias-region physical mapping in the full process, but the retained-function-table audit currently only supports a small committed startup set (`4 KiB` low + up to `15 MiB` physical + full scanned image/function table). Next step is to turn this into a real demand/page-backed guest memory design; full eager low/physical mapping still exceeds the current practical budget.
- A separate startup-memory audit now proves an even narrower early-runtime milestone: `0x30000` low memory + `15 MiB` physical heap + Xenon fixed memory mapping can complete `KiSystemStartup()` heap/Xenon initialization when host config/content/module loading and generated function-table insertion are intentionally skipped.
- The host config-path blocker after that milestone was narrowed: `Config::Save()` had generated duplicate `[Input]` TOML tables, causing `toml::parse()` to throw and hit Ryujinx's GCC unwinder `gcspr_el0` limitation. The current code rewrites duplicate-table configs before parsing and writes grouped TOML sections. Next runtime work can continue toward content/module preflight, still without entering guest code until guest memory is ready.
- The next content-path audit is now narrowed to `Installer::checkGameInstall()`: `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_INSTALL_CHECK` runs `Config::Load()`, checks the expected SD module path, and stops before host startup, module loading, update checking, video/audio setup, or guest code. This validates path/layout plumbing only; it is not a gameplay test.

## Reference Repos

- `Jenish094/GTA4Recomp`: useful for GTA IV recompilation config ideas, but less complete.
- `hedge-dev/UnleashedRecomp`: useful as a complete recomp project reference, but it is not GTA IV and not a Switch port.
- `givethesourceplox/UnleashedRecomp-NX`: useful as a Switch recomp project reference for devkitA64/libnx build conventions.

## Collaboration Notes

- The user wants Chinese communication.
- Be direct but careful: this is reverse engineering and porting work, not ordinary app compilation.
- The Switch target matters in every build-system and runtime decision.
- Prefer conservative, isolated patches that reveal the next blocker.
