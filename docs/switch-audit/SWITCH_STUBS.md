# Switch Stub Ledger

This file tracks temporary Switch audit stubs that must be replaced or removed before the port can be considered playable.

## Current Rule

These stubs are compile/audit scaffolding only. Do not treat them as working gameplay systems.

## Runtime / ReXGlue

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecompLib\rexglue_runtime_stubs.cpp`

- Switch-only `rex::chrono::Clock::QueryGuestTickCount()` uses `std::chrono::steady_clock` and a rough 50 MHz tick conversion. Replace with the final guest timebase/runtime clock design.
- Switch-only `rex::thread::global_critical_region::mutex()` returns a process-local `std::recursive_mutex`. Audit against final thread/runtime model.
- Switch-only `rex::GetLoggerRaw()`, `rex::GetLogger(LogCategoryId)`, and `rex::GetLogger()` return null/shared-empty logger handles. Replace with real ReXGlue logging integration or remove once full ReXGlue runtime links.
- Switch-only `rex::runtime::MMIOHandler` implementation is inert: `Install()` returns null, `RegisterRange()` fails, `LookupRange()` returns null, load/store checks return false. Replace with real MMIO/page-fault handling if needed by generated runtime.
- Switch-only `rex::g_current_ppc_context` and `rex::g_memory_base` are defined locally to satisfy TLS symbols. Remove or reconcile if the real ReXGlue `ppc_types.cpp` / runtime sources are linked.
- Switch-only `rexcrt_*` string/memory hooks are partial native guest-memory helpers:
  - `rexcrt_strstr`
  - `rexcrt__stricmp`
  - `rexcrt_strchr`
  - `rexcrt_strncpy`
  - `rexcrt_strncmp`
  - `rexcrt_strtok`
  - `rexcrt_strrchr`
  - `rexcrt_memcpy`
  - `rexcrt_memset`
  - `rexcrt_XMemCpy`
- Switch-only `rexcrt_*` file, directory, heap, and file-time hooks are audit no-ops/failure stubs:
  - `rexcrt_CreateFileA`
  - `rexcrt_CloseHandle`
  - `rexcrt_GetFileSize`
  - `rexcrt_SetFilePointer`
  - `rexcrt_ReadFile`
  - `rexcrt_WriteFile`
  - `rexcrt_GetFileSizeEx`
  - `rexcrt_SetFileTime`
  - `rexcrt_GetFileAttributesA`
  - `rexcrt_SetFileAttributesA`
  - `rexcrt_SetEndOfFile`
  - `rexcrt_DeleteFileA`
  - `rexcrt_MoveFileA`
  - `rexcrt_CreateDirectoryA`
  - `rexcrt_RtlSizeHeap`
  - `rexcrt_RtlAllocateHeap`
  - `rexcrt_RtlFreeHeap`
  - `rexcrt_RtlReAllocateHeap`
  - `rexcrt_FlushFileBuffers`
  - `rexcrt_FindFirstFileA`
  - `rexcrt_FindNextFileA`
  - `rexcrt_FindClose`
  - `rexcrt_RemoveDirectoryA`
  - `rexcrt_SetFilePointerEx`
  - `rexcrt_GetFileAttributesExA`
  - `rexcrt_CompareFileTime`
  - `rexcrt_CopyFileA`
  - `rexcrt_GetFileType`
  - `rexcrt_GetDiskFreeSpaceExA`

Replacement direction: prefer real ReXGlue CRT runtime sources where possible, but only after checking dependencies on `kernel_state`, guest memory layout, VFS, logging, heap allocator, and Switch filesystem semantics.

## OS / UI / Installer Platform Stubs

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\os\switch\registry_switch.inl`

- `os::registry::Init()`, `ReadValue()`, and `WriteValue()` are no-op/false. Replace with a Switch file-backed settings store under `sdmc:/switch/LibertyRecomp`.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\install\platform_paths.cpp`

- Switch install/home path is hardcoded to `sdmc:/switch/LibertyRecomp`.
- Bundled AES key lookup checks simple `romfs:/...` candidates and falls back to the SD install path.
- CMake now packages an NRO with a build-directory `romfs` placeholder, but the directory is currently empty.
- Current `main.cpp` audit build mounts the default `romfs:` device with `romfsMountSelf("romfs")`, checks whether `sdmc:` is already registered before calling `fsdevMountSdmc()`, appends startup/content-preflight diagnostics to `sdmc:/switch/LibertyRecomp/LibertyRecomp.log`, and unmounts only devices it mounted itself.
- Current `main.cpp` audit build skips `romfsMountSelf()` when libnx `envIsNso()` reports true, because the devkitPro ExeFS PFS0 audit packages do not provide a RomFS data storage and Ryujinx throws internally when `romfsMountSelf()` is attempted in that package form. SD remains the active audit content path.
- `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONFIG_LOAD` is a Switch-only audit stop that runs `Config::Load()` after SD/RomFS startup and then stops before content preflight, host startup, module loading, or guest code. It exists to isolate host config behavior and is not a release/runtime feature.
- A visible boot-probe test showed `fsdevMountSdmc()` returning `0x00000559` on both hardware and Ryujinx. The corrected probe showed this is compatible with `sdmc:` already being registered; file writes work when the existing device is used.
- Missing `sdmc:/switch/LibertyRecomp/game/default.xex` now stops on a visible `Missing game content` diagnostic before host startup or guest code. If `default.xex` exists while Switch guest memory is still disabled, the build stops on a visible `Guest memory disabled` diagnostic before `KiSystemStartup()`.
- Full LibertyRecomp now has both `LibertyRecompNro` and `LibertyRecompExeFs` audit packaging targets. `LibertyRecompExeFs` uses `os/switch/liberty_recomp_npdm.json` with `system_resource_size=0x10000000`, but guest memory remains disabled by default.
- A separate full-process guest-memory audit build is configured under:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-guest-memory-audit-debug`
  It enables `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT` and `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP` for ExeFS/NPDM only. This build maps audit windows, can skip or retain function mapping initialization via `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS`, reaches `main()`, and stops before content preflight/host startup.
- Switch audit diagnostics also send lines through `svcOutputDebugString()` when possible.
- This is acceptable for audit builds, but final packaging must define real RomFS/SD layout, content discovery, error behavior, and migration behavior.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\os\switch\audit_diagnostics_switch.cpp`

- Switch-only visible audit diagnostic screen using libnx `consoleInit()`/`consoleUpdate()`.
- The helper no longer accepts controller input or auto-exits; use HOME/close software to leave it during audit tests.
- This is a temporary diagnostic shell for audit builds. Replace with a real Switch first-run/error UI once the final packaging and content installer story is defined.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\os\switch\boot_probe_switch.c`

- Minimal independent Switch boot probe used to separate launch-environment failure from LibertyRecomp runtime failure.
- It is packaged as `LibertyRecompBootProbe.nro`, shows a visible console screen with `sdmc:` before/after state, `fsdevMountSdmc()`, `mkdir()`, `stat()`, and `fopen()` results, writes `sdmc:/switch/LibertyRecomp/boot_probe.log` when possible, and then sleeps indefinitely.
- It does not link LibertyRecomp sources or run the large C++ global/static initialization path.
- Remove or keep it excluded from release packaging after the launch-path audit is complete.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\os\switch\memory_probe_switch.c`

- Independent Switch guest-memory SVC probe packaged as `LibertyRecompMemoryProbe.nro` and `LibertyRecompMemoryProbeExefs.nsp`.
- It logs `svcGetInfo()` memory/system-resource values and tests `svcMapPhysicalMemory()` against ASLR reservations, heap-region candidates, and Alias region candidates.
- Latest Ryujinx result with `__nx_heap_size=96 MiB`: ordinary memory remains available (`UsedMemorySize=0x06143000`), but `SystemResourceSizeTotal=0` and `SystemResourceSizeUsed=0`; all `svcMapPhysicalMemory()` attempts return `0x0000FA01`.
- Latest Ryujinx ExeFS/NPDM result with `system_resource_size=0x08000000`: `SystemResourceSizeTotal=0x08000000`, `SystemResourceSizeUsed=0x1000`; ASLR/heap mappings return `0x0000DC01`, while Alias region mappings at `0x80000000` and `0x102000000` return success and are writable.
- This target does not run game code. Keep it as an audit probe until guest-memory packaging/runtime design is resolved, then remove it from release packaging.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\install\installer_switch_stub.cpp`

- `Installer::checkGameInstall()` is no longer a blind success stub. It sets `modulePath` to `default.xex` under the embedded game root or `sdmc:/switch/LibertyRecomp/game/default.xex`, then uses `fopen()` to check whether that module exists.
- The rest of the installer implementation remains a Switch audit stub: DLC checks still return simplified success, integrity checks do not validate content, and install/copy/parse operations are not implemented.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\ui\installer_wizard_switch_stub.cpp`

- `InstallerWizard::Run()` now reports that the Switch installer wizard is not implemented and returns false instead of pretending installation succeeded.
- This prevents missing-content audit runs from falling through as if the desktop installer had completed. A real Switch content installer or first-run diagnostic UI is still needed.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\os\switch\process_switch.cpp`

- Process launching is no-op because Switch homebrew cannot freely spawn desktop child processes.
- Executable path/root/working directory are simple RomFS/SD guesses. Revisit with final NRO/RomFS packaging.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\user\paths.cpp`

- Switch startup avoids pre-main `std::filesystem::exists()` checks against `romfs:` because RomFS is not mounted until `main()`. `GetUserPath()` currently returns a lazy `sdmc:/switch/LibertyRecomp` path. Revisit once final RomFS/SD layout and first-run behavior are defined.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\user\config.cpp`

- `Config::Save()` now writes visible config definitions grouped by TOML section so the GTA IV binding layout does not emit duplicate `[Input]` tables.
- `Config::Load()` checks for duplicate TOML tables before calling `toml::parse()` and rewrites defaults if an old invalid config is found. This avoids the known Ryujinx/GCC 15 unwinder path for this specific parse-error case.
- Switch-only config breadcrumbs use `svcOutputDebugString()` for audit runs. This is diagnostic instrumentation; final user-facing config/error behavior still needs a real Switch UI policy.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\install\update_checker.cpp`

- Switch `visitWebsite()` is a no-op. Replace with an in-app message, QR/link display, or disable desktop update UX on Switch.

## Networking

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\kernel\io\p2p_manager_switch_stub.cpp`

- Switch audit stub for GameNetworkingSockets symbols:
  - `GameNetworkingSockets_Init`
  - `GameNetworkingSockets_Kill`
  - `SteamNetworkingSockets_LibV12`
  - `SteamNetworkingUtils_LibV4`
- `GameNetworkingSockets_Init()` returns false so `P2PManager` remains uninitialized and P2P branches in socket/voice code should not run.
- Replace by porting/linking GameNetworkingSockets on Switch or by implementing a Switch-specific networking/session backend.

## Audio

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\apu\xma_decoder_switch_stub.cpp`

- XMA/FFmpeg decode path is a Switch stub. Real game audio requires a proper XMA decode path or content/audio conversion strategy.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecompLib\rexglue_runtime_stubs.cpp`

- ReXGlue `AudioSystem` and `XmaDecoder` methods return inert success/failure values. Replace with the final audio backend and decoder integration.

## Graphics

Plume/Vulkan Switch backend remains stubbed in the current audit branch.

- Real Switch rendering still needs a deko3d backend or another Switch-specific renderer.
- Any SDL/window success at compile time should not be interpreted as a working renderer.

## Guest Memory / Protection

Current Switch guest memory/page-protection work is audit scaffolding.

- File mapping falls back to read-into-memory instead of desktop mmap.
- `mprotect`/page protection behavior is not final.
- `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\kernel\memory.cpp` contains a Switch-only sparse guest memory audit backend behind `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT`:
  - queries the process Alias region with `svcGetInfo()` and uses it as the 4 GiB PPC guest base,
  - maps only selected physical pages with `svcMapPhysicalMemory()`,
  - maps selected audit windows in the Alias region; the current retained-function-table audit defaults are `4 KiB` low memory, the `64 KiB` XMA I/O window, a `15 MiB` physical heap window from `0x80000000`, and the full scanned XEX image/function-table range,
  - overrides the full app libnx/newlib `__nx_heap_size` to 16 MiB so startup does not consume the process budget before guest ranges are mapped.
- The sparse backend is disabled by default for the current startup-container build because Ryujinx returned kernel `0x0000FA01` (`InvalidState`) on the first low-heap `svcMapPhysicalMemory()` call. The independent memory probe confirmed this is not fixed by choosing a smaller range, reserving differently, mapping inside the heap region, or reducing the libnx heap to 96 MiB: the current NRO/hbloader environment reports `SystemResourceSizeTotal=0`, and every tested `svcMapPhysicalMemory()` path still returns `0x0000FA01`.
- The ExeFS/NPDM memory probe confirms the next constraints: NPDM `system_resource_size` removes the `0x0000FA01` blocker, and `svcMapPhysicalMemory()` must target the Alias region to avoid `0x0000DC01`.
- Full-process guest-memory audit results now refine that constraint: a 96 MiB total audit mapping succeeds in Ryujinx (`32 MiB` low memory, `64 KiB` XMA I/O, `32 MiB` physical heap, `32 MiB` image/function-table audit space), but the previous eager low-memory attempt at `64 MiB` fails with kernel `0x0000D001` (`OutOfMemory`) before `main()`.
- `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS` now separates two audit behaviors that were previously tied together: `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP` still stops in `main()`, while the new skip flag controls whether `Memory::Memory()` returns before generated function-table initialization.
- When function mappings are not skipped, the Switch sparse backend scans `PPCFuncMappings` to size the image/function-table mapping from the actual highest mapped guest address. This fixed a Ryujinx invalid access at `0x1043D9000` caused by import/function mappings extending beyond `PPC_CODE_BASE + PPC_CODE_SIZE`.
- Ryujinx confirmed a narrow function-table audit can map `4 KiB` low memory, `64 KiB` XMA I/O, `4 KiB` physical heap, and the full scanned image/function-table range (`0x24A0000` bytes), then complete `Memory::Memory()` function-table insertion and reach the `main()` audit stop. This proves function-table initialization can run when the mapped range covers the actual `PPCFuncMappings` maximum guest address (`0x82A77E28`).
- Per-map SystemResource logging is now enabled for Switch sparse guest-memory audits. In Ryujinx, `SystemResourceSizeUsed` stayed `0x1000` across successful and failed mappings, so that counter does not explain the current `0x0000D001` failures.
- The same full-process build still cannot eagerly map both the `32 MiB` low window and `32 MiB` physical heap once function mappings are retained and the full image/function-table range is enabled; Ryujinx returns kernel `0x0000D001` (`OutOfMemory`) at the physical heap map even with full-app NPDM `system_resource_size=0x10000000`.
- With `4 KiB` low memory and the full scanned image/function-table mapping retained, capacity testing confirmed physical heap windows of `8 MiB`, `12 MiB`, `14 MiB`, and `15 MiB` succeed, while `16 MiB` fails. The current CMake audit defaults are `LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE=0x1000` and `LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE=0xF00000`.
- `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT` is a narrower startup audit mode. It maps `0x30000` low memory, the `64 KiB` XMA I/O window, a `15 MiB` physical heap window, and the Xenon fixed memory span `0x82000000..0x831F0000`; it skips generated function-table insertion and bypasses host config/content/module loading, then runs `KiSystemStartup()` only through `g_userHeap.Init()` and `InitializeXenonMemoryRegions()`.
- Ryujinx confirmed the startup-memory/Xenon-init audit Build ID `539c9829646e4f6e847b882ee8468d40cc579702` reaches the visible audit stop after heap/Xenon initialization. This is still not guest code and not playable.
- The `Config::Load()` host-config blocker was narrowed to an invalid generated TOML file: `Config::Save()` previously emitted duplicate `[Input]` tables, which made `toml::parse()` throw and enter Ryujinx's GCC 15 unwinder/toolchain blocker (`Unknown MRS ... gcspr_el0`). The current config-audit build rewrites that old file before parsing, and a second run parses successfully.
- Default Switch startup builds now leave `Memory::base=nullptr` and stop in `main()` before any guest-memory use.
- `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\kernel\heap.cpp` caps Switch `o1heap` arenas to the sparse mapped audit windows.
- This replaced the previous Switch `new (std::nothrow) uint8_t[4 GiB]` allocation, which failed during global `Memory g_memory` construction and entered GCC's C++ unwinder on hardware.
- Guest memory backing store still needs a real Switch runtime design before gameplay testing is meaningful. The current generated PPC runtime passes a raw contiguous `uint8_t* base` into generated functions, so a segmented backing store behind `Translate()` alone is not enough without broader generated-code/runtime ABI changes. The current practical branch is a title/package path that provides NPDM system-resource budget and uses the Alias region for sparse physical mapping; the fallback branch is a larger memory ABI redesign.

## Linkage / ABI Fixes

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\toolchains\switch-libnx.cmake`

- Switch configure generates `liberty-switch.specs` in the build directory and a build-local copy of `libnx/switch.ld`.
- This avoids `switch.specs` falling back to `/opt/devkitpro/libnx/switch.ld` when Ninja is launched without `DEVKITPRO` in the process environment.
- Switch link now searches `C:\devkitPro\devkitA64\aarch64-none-elf\lib\pic` before the non-PIC devkitA64 library directory so libstdc++/newlib C++ metadata does not require dynamic relocations in the read-only LOAD.
- Generated specs include `-z text` and `-z gcs=never`. `-z text` is now preserved; `-z gcs=never` marks the output as not using Arm GCS, but it does not remove GCC 15 libgcc's internal `gcspr_el0` instruction from the unwinder.
- Keep this audited against devkitPro updates; it duplicates the current libnx `switch.specs` link/startfile fragments.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\CMakeLists.txt`

- Switch link no longer adds `-Wl,-z,notext`.
- `LibertyRecompNro` is an audit packaging target that runs `nacptool` and `elf2nro`, embeds the project `game_icon.png`, and uses an empty placeholder RomFS; it does not imply a playable Switch runtime.
- Initial `readelf -rW` audit found `.relr.dyn` relocated `63,326` locations: `1,662` in the read-only LOAD before RELRO overlap, `4,975` in the RELRO overlap, and `56,689` in writable memory. The first read-only examples were libstdc++ RTTI/vtable symbols such as `_ZTISt9bad_alloc` and `_ZTVSt9bad_alloc`.
- Current audit build links with `-z text` successfully. Final `readelf -dW` has no `TEXTREL`; final RELR target classification is `LOAD0 R E: 0`, `LOAD1 R: 0`, `LOAD2 RW: 63,667`, `outside: 0`.
- User-provided hardware crash `01781420466_010000000000100d.log` matched the old debt: Atmosphere `Data Abort`, PC `LibertyRecomp + 0x8c2ac20`, LR `LibertyRecomp + 0x8c2aab8`, base register `LibertyRecomp + 0x0`, and relocation offset `0x8e97010`, consistent with libnx startup applying RELR into old read-only metadata.
- Ryujinx Canary 1.3.269 now gets past the old loader/RELR failure but aborts in its JIT on `Unknown MRS 0xD53B2521 at 0x00000000111B19B8`. With the NRO loaded at `0x8500000`, this maps to ELF offset `0x8cb19b8`, `_Unwind_RaiseException_Phase2`, instruction `mrs x1, gcspr_el0` from GCC 15 libgcc. Treat this as a simulator/toolchain blocker, not evidence that the old hardware RELR crash remains.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\kernel\function.h`

- `GUEST_FUNCTION_HOOK` and `GUEST_FUNCTION_STUB` were changed to emit `extern "C"` definitions via `PPC_FUNC_IMPL` to match generated `PPC_EXTERN_IMPORT` declarations.
- This is an ABI fix, not a stub, but keep it in mind when reviewing generated import linkage.

File: `C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\LibertyRecomp\kernel\xam.cpp`

- `__imp__XamUserGetSigninState` was changed to `PPC_FUNC_IMPL` for the same C linkage reason.
- Function behavior still returns a simplified signed-in state and should be audited later.
