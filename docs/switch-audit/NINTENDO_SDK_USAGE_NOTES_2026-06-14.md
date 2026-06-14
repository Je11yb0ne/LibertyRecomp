# Nintendo SDK Local Asset Usage Notes - 2026-06-14

## Scope

Local path inspected:

`D:\Nintendo`

This directory contains official Nintendo Switch SDK packages, NX add-ons, documentation, samples, tools, and related libraries. Treat it as licensed/proprietary material. Do not copy Nintendo SDK headers, source, sample code, or binaries into the LibertyRecomp repository. Use it as local reference material unless an explicit separate licensed official-SDK build target is created.

Current LibertyRecomp Switch target remains a devkitPro/libnx homebrew audit target that emits an NRO. It should not directly link official Nintendo SDK `.lib`, `.a`, `.nso`, or `.nss` artifacts.

## High-Value Local Paths

- Main official SDK:
  `D:\Nintendo\NintendoSDK-20221122_0843-en_v2\NintendoSDK`
- NX add-on SDK:
  `D:\Nintendo\NintendoSDK-NXAddon-20221122_0843-en_v3\NintendoSDK`
- API/documentation tree:
  `D:\Nintendo\NintendoSDK_Documents-ForNX-20221122_0843-en_v3\NintendoSDK\Documents`
- NX add-on samples:
  `D:\Nintendo\NintendoSDK_Samples-NXAddon-20221122_0843-en_v3\NintendoSDK\Samples\Sources\Applications`
- Official NX Clang/toolchain package:
  `D:\Nintendo\Clang-ForNX-20220720_1508_v3\NintendoSDK\Compilers\NX`
- Breakpad package:
  `D:\Nintendo\Breakpad-ForNX-20200601_2043_v2\NintendoSDK`
- Capstone package:
  `D:\Nintendo\Capstone_ForNX-20170615_2321_v9\NintendoSDK`

## What Is Directly Useful Now

- Documentation and samples are useful for subsystem design and behavior checks.
- Host-side tools may be useful for official SDK artifacts/logs:
  - `CrashReport.exe`
  - `ConvertNxbinlog.exe`
  - `MakeMeta.exe`
  - `HidShell.exe`
  - `BackupSaveData.exe`
- Current Atmosphere `.log` files are still better handled with devkitA64 `addr2line`/`objdump`, because they already include module offsets.
- Current NRO metadata should continue to use devkitPro `nacptool` and `elf2nro`; official `MakeMeta.exe` is for official SDK metadata flows, not the current homebrew packaging path.

## What Not To Do

- Do not mix official SDK headers/libraries with the current libnx homebrew target.
- Do not copy SDK sample code into the repository.
- Do not make the current `switch-libnx.cmake` depend on `D:\Nintendo`.
- Do not treat official SDK package/NPDM/NACP flows as proof that the current NRO is playable.

Reasons:

- Official SDK targets and libnx homebrew targets have different startup, ABI, service, metadata, and packaging assumptions.
- Official SDK libraries target official NX application builds; the current target is a libnx NRO audit artifact.
- Copying proprietary SDK code into this repo would make the working tree harder to share and audit.

## Mapping To Current LibertyRecomp Gaps

Graphics:

- Relevant samples:
  - `GfxSimple`
  - `GfxShaderCompile`
  - `GfxSimpleCompute`
  - `NvnSimple01ScreenSpaceTriangles` through `NvnSimple08BindlessCube`
  - `NvnTutorial01GettingStartedWithNVN` through `NvnTutorial15HLSL`
- Use only as conceptual reference for GPU memory ownership, frame pacing, command submission, shader variants, render targets, and pipeline state.
- Current homebrew implementation should still use deko3d/libnx or another homebrew-compatible backend, not official NVN.

Audio:

- Relevant samples:
  - `AudioOut`
  - `AudioOutWithResampler`
  - `AudioRenderer`
  - `AudioMemoryPool`
  - `AudioDevice`
  - `AudioPerformanceMetrics`
  - `AudioToolUtilities`
- Relevant tools:
  - `AdpcmEncoder.exe`
  - `OpusEncoder.exe`
  - `OpusDecoder.exe`
  - `SystemAudioMonitor.exe`
  - `AudioRedirector.exe`
- Use as reference for audio buffer cadence, sample format, memory pools, renderer limits, and performance expectations.
- Current LibertyRecomp XMA path still needs a real decode/conversion strategy; SDK audio tools do not directly decode GTA IV XMA.

Filesystem, RomFS, SD, save data:

- Relevant samples:
  - `FsRom`
  - `FsSdCardForDebug`
  - `FsHost`
  - `FsRamDisk`
  - `FsFileDataCache`
  - `FsSaveDataEnsuredByApplication`
  - `FsSaveDataEnsuredBySystem`
  - `FsSaveDataForDebug`
  - `FsSetAllocator`
- Use as reference for final RomFS/SD layout, content discovery, save storage model, and error behavior.
- Current NRO still embeds an empty placeholder RomFS and uses `sdmc:/switch/LibertyRecomp` guesses.

Input/HID:

- Relevant samples:
  - `HidSimple`
  - `HidNpadSimple`
  - `HidNpadIntegrate`
  - `HidNpadSixAxisSensor`
  - `HidControllerSequence`
  - `HidVibrationBasic`
  - `HidVibrationGenerator`
  - `HidVibrationPlayer`
  - `HidVibrationRealTimeConverter`
- Relevant tool:
  - `HidShell.exe`
- Use as reference for controller modes, vibration expectations, and six-axis behavior.
- Current homebrew target should map these concepts to SDL2/libnx input, not official `nn::hid` headers.

OS, memory, runtime:

- Relevant samples:
  - `OsMemoryHeap`
  - `OsVirtualAddressMemory`
  - `OsThread`
  - `OsThreadLocalStorage`
  - `OsMutex`
  - `OsEvent`
  - `OsConditionVariable`
  - `OsUserExceptionHandler`
  - `OsTickAndTimeSpan`
- Use as reference for guest memory/page-protection design, thread/TLS assumptions, crash handling, and timing.
- Current guest memory/page protection remains audit scaffolding and needs a real Switch runtime design.

Networking:

- Relevant samples:
  - `NifmNetworkConnectionSimple`
  - `SocketBasic`
  - `SocketResolver`
  - `SocketEventFd`
  - `SocketMulticast`
  - `SocketSendMMsg`
  - `WebSocketChat`
  - `WebSocketEcho`
- Use as reference for service initialization, socket behavior, and network permission expectations.
- Current GameNetworkingSockets/P2P remains disabled by the Switch audit stub.

Crash/debug:

- Relevant tools/packages:
  - `CrashReport.exe`
  - `ConvertNxbinlog.exe`
  - `Breakpad-ForNX`
  - `Capstone_ForNX`
  - `NintendoCpuProfiler`
  - `NintendoSDK_TargetManager2`
- Official tools may help with official target logs/dumps or devkit workflows.
- For Atmosphere crash reports from the current NRO, continue mapping offsets with devkitA64 `addr2line`/`objdump`.

## Current Ryujinx GCS Blocker

The current Ryujinx Canary 1.3.269 failure is:

`Unknown MRS 0xD53B2521`

The instruction maps to GCC 15 libgcc `_Unwind_RaiseException_Phase2`, `mrs x1, gcspr_el0`.

The official `Clang-ForNX` package may be useful for future official-SDK experiments or binary analysis, but it should not be substituted into the current libnx NRO toolchain without a separate target. It uses Nintendo NX triples and official SDK runtime assumptions, not the devkitPro/libnx homebrew environment.

## Recommended Use Plan

1. Keep current `switch-libnx.cmake` as the homebrew audit target.
2. Use `D:\Nintendo` documentation/samples as local reference when replacing specific stubs.
3. Do not include or link proprietary SDK material in the repo.
4. If an official SDK build is ever needed, create a separate `switch-nintendo-sdk.cmake` target and keep it isolated from the libnx NRO path.
5. Prioritize reference review in this order:
   - `OsVirtualAddressMemory` / `OsMemoryHeap` for guest memory.
   - `FsRom` / `FsSdCardForDebug` / save-data samples for final storage layout.
   - `AudioOut` / `AudioRenderer` / `AudioMemoryPool` for audio backend behavior.
   - `NvnTutorial*` / `GfxSimple` for renderer architecture notes.
   - `HidNpad*` / vibration samples for controller parity.
   - `Socket*` / `NifmNetworkConnectionSimple` for networking replacement planning.
