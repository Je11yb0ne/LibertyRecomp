# LibertyRecomp Switch Continuation Guide

## Current Mainline Goal

Mainline work is now Windows runtime bring-up first, with ReXGlue as the primary runtime/codegen reference and XenonRecomp as an auxiliary translation/reference aid. The current Windows objective is to move the legacy LibertyRecomp bootstrap through guest startup blockers by fixing wrapper recursion, MMIO handling, VBlank/thread startup, module initialization, guest memory/page backing, content paths, and startup logging one verified boundary at a time.

Switch work is paused at the verified LibertyRecompExeFs / NSP-like pre-guest audit baseline. Use Switch only for regression checks, packaging/content-layout fixes, or deliberately scoped pre-guest blockers while Windows runtime work is the mainline. This is not a playable Switch port; all current Switch artifacts are audit packages.

## Tooling Priority

Use ReXGlue SDK as the primary runtime/codegen reference for ongoing LibertyRecomp / GTA IV work. The current project is already built around ReXGlue generated sources under `glue/rexglue-sdk-main/gta4-recomp/generated`, and wrapper/debugging work should treat generated `__imp__sub_x` implementations as the source of truth when a project-side strong wrapper hooks `sub_x`.

Use XenonRecomp as an upstream translation/reference aid, not the main runtime integration target. Do not modify `tools/XenonRecomp` unless a stage explicitly proves a translation-side change is required and records why first.

Local read-only references:

- ReXGlue SDK: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\rexglue-sdk`
- Built ReXGlue SDK 0.8.1.32-dev.gf22cd9d: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest\tools\rexglue-sdk-0.8.1.32-dev.gf22cd9d-win-amd64`
- Recompiled GTA IV samples: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\Recompiled-Samples`
- XenonRunner runtime sample: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\XenonRunner`

The built ReXGlue CLI is usable now for experiments: `...\bin\rexglue.exe --version` reports `0.8.1.32-dev.gf22cd9d`, and `codegen` accepts the existing `glue\rexglue-sdk-main\gta4-recomp\gta4_config.toml`. Do not overwrite the tracked `generated` directory directly. First run any 0.8.1 codegen attempt into a temporary output directory and compare the generated function graph/import table against the currently building sources. Use it immediately for read-only CLI inspection, codegen dry-run/diff work, or a future ReXApp migration branch; continue using the current generated `__imp__` sources for the active wrapper-recursion startup chain.

Fresh ReXGlue binary smoke on 2026-06-17: copied `gta4_config.toml` to `%TEMP%`, rewrote `file_path`, `patched_file_path`, and `out_directory_path` to temporary/absolute paths, and ran `rexglue --force codegen <temp-config>`. Result: exit `0`, generated `86` files in `C:\Users\JELLYB~1\AppData\Local\Temp\rexglue-codegen-smoke-cc7eaf6f-1dbf-429b-8209-26ae79dbbe7a\generated`, and completed in `45.6s`. The CLI migrated the old config to a temp `gta4_manifest.toml`, warned that legacy `patched_file_path` is no longer used, reported several current config addresses not in any code region, emitted unresolved branch diagnostics, and generated a stub for `0x82A77E28`. Treat these as diff/audit inputs before any tracked generated-code refresh.

## Current Stage Goal

Mainline work has pivoted back to Windows runtime / unfinished upstream code. Switch work remains paused at the verified pre-guest baseline and should be used only for regression checks, packaging/content-layout fixes, or deliberately scoped pre-guest blockers.

The current Windows runtime bring-up target is to keep the legacy LibertyRecomp bootstrap aligned with ReXGlue generated code. ReXGlue generated `__imp__sub_x` implementations are the source of truth when project-side strong wrappers hook a generated function.

Fresh Windows stage result:

- Build directory: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest`
- Build command: `ninja -C ...\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result: success; the only reported warnings are the existing `vfs.h` block-comment warning, `imports.cpp` `uint32_t >= 4294967296` tautology, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Runtime bridge added: Windows legacy `Memory g_memory` bootstrap now installs a ReXGlue `MMIOHandler` bridge before guest startup and registers deterministic audit ranges for GPU `0x7FC80000`, XMA `0x7FEA0000`, and the broad Xbox MMIO window `0x7F000000..0x7FFFFFFF`.
- Smoke evidence: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-ea18ff7d-16d0-4b66-80e6-578144840d5d.log` and `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-540e1019-b20d-4d67-ab0f-cf0e12d10fdf.log`.
- Smoke result: process still exits with `0xC00000FD` stack overflow, but it now reaches `[MMIO-BRIDGE] write addr=0x7FC80714 reg=0x01C5 ...`, so the previous ReXGlue `MMIOHandler::CheckStore/CheckLoad` access violation is cleared.
- Wrapper fixes in this stage include `sub_82850630`, `sub_829CB140`, `sub_829CAE68`, `sub_829D5948`, `sub_82851DD8`, `sub_8285BDC8`, `sub_8285BC60`, `sub_827DFE10`, `sub_8284FAD8`, and `sub_82859B80`, all redirected to generated `__imp__` implementations.
- Current blocker after fresh smoke: stack overflow at RVA `0x6EEBA`, mapped by the current link map to `imports.cpp.obj` `sub_82857240 + 0x6A`.

Next Windows boundary: fix `sub_82857240` only after confirming a generated `__imp__sub_82857240` exists, then rebuild and rerun the same smoke. Continue using this targeted pattern rather than broad automatic conversion.

2026-06-16 Windows continuation update:

- ReXGlue wiki review completed for `Runtime-Architecture-Overview.md`, `Generated-Code-Structure.md`, `Function-Overrides.md`, `Memory.md`, `Virtual-File-System.md`, `ReXApp.md`, and `Codegen-Pipeline-Overview.md`.
- ReXGlue confirms the intended model: generated functions expose weak public aliases and strong `__imp__` implementations; project-side strong wrappers must call `__imp__sub_x` when they wrap rather than replace the generated body.
- Windows legacy bootstrap work in this repo is still not a clean ReXApp migration. It currently bridges the old LibertyRecomp startup toward the ReXGlue generated/runtime model one blocker at a time.
- Vulkan is the preferred Windows renderer direction for future Switch portability because ReXApp can inject graphics backends and because Vulkan-like renderer boundaries will be easier to translate toward a Switch-specific backend than D3D12-only work. Do not assume desktop Vulkan code can run on Switch unchanged; Switch still needs a real deko3d/NVN-style backend or a deliberate portability layer.
- Additional wrapper call-through fixes verified in this continuation: `sub_827EED88`, `sub_827EB748`, `sub_827EAE38`, and `sub_82897760` now call their generated `__imp__` implementations instead of recursively calling themselves.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-0b1873c9-cee7-4265-aea1-9cf4835e8bbd.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-ae94211f-8079-4d93-8d68-d72e9c69b7e1.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past the fixed wrapper chain. Final smoke reached the MMIO bridge, VBlank callback registration, two GPU MMIO writes, and VBlank tick `#1`. Latest repeated frame is RVA `0x6656A`; subtract the PE/map `0x1000` delta to map offset `0x6556A`, which falls in `imports.cpp.obj` `sub_827EA150`.
- Next Windows boundary: confirm whether `sub_827EA150` is another project-side wrapper self-call with a generated `__imp__sub_827EA150`, then make one minimal fix and rerun the same build/smoke. If it is not a simple wrapper recursion, stop and trace the call path instead of guessing.
- Parallel work guidance: another conversation may start Windows-side planning or renderer/Vulkan research now, but should avoid editing `LibertyRecomp/kernel/imports.cpp`, `LibertyRecomp/kernel/memory.cpp`, `LibertyRecomp/kernel/memory.h`, and `LibertyRecomp/main.cpp` until this wrapper/MMIO startup stage is committed and pushed. Safe parallel areas are read-only ReXGlue wiki/source review, a renderer design note, IDA/rexglue symbol investigation, or Windows build/run documentation.

2026-06-17 Windows continuation update:

- Mainline goal was explicitly changed to Windows-first runtime bring-up. Switch remains frozen at the verified LibertyRecompExeFs / NSP-like pre-guest baseline except for regression checks or narrowly scoped Switch-only blockers.
- Confirmed root cause for the next stack overflow frame: `LibertyRecomp/kernel/imports.cpp` had a project-side strong wrapper for `sub_827EA150` that called the public alias `sub_827EA150(ctx, base)` instead of the generated implementation. ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_827EA150)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.52.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: `sub_827EA150` now declares and calls `__imp__sub_827EA150`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-54247445-2615-4a33-b1d7-7f8419150ec7.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-2dd943a9-52af-4964-bb9a-91f99f3c0fc1.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_827EA150`. Smoke reached the MMIO bridge, graphics backend attempts, VBlank tick `#2`, and the `sub_8284F880` exit trace. Latest repeated frame is RVA `0x7824A`; subtract the PE/map `0x1000` delta to map offset `0x7724A`, which falls in `imports.cpp.obj` `sub_827827C8 + 0x6A`.
- Next Windows boundary: confirm whether `sub_827827C8` is another project-side wrapper self-call with a generated `__imp__sub_827827C8`, then make one minimal fix and rerun the same build/smoke. If it is not a simple wrapper recursion, stop and trace the call path instead of guessing.

2026-06-17 Windows wrapper continuation:

- Confirmed `sub_827827C8` had the same wrapper-recursion pattern. The project-side wrapper called `sub_827827C8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_827827C8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.49.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_827827C8(...)` declaration for later forward declarations/callers, added `extern "C" void __imp__sub_827827C8(...)`, and changed only the logging wrapper body to call `__imp__sub_827827C8(ctx, base)`. The later `sub_822F8890` call to public `sub_827827C8(ctx, base)` intentionally remains so it can enter the wrapper.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-aca6d4b0-2007-4690-8eac-479e1bd1dee4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-1f9e3f70-81a2-4375-88a2-cf5b2bf6d7e9.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_827827C8`. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, and `sub_8284F880` exit. Latest repeated frame is RVA `0x7831A`; subtract the PE/map `0x1000` delta to map offset `0x7731A`, which falls in `imports.cpp.obj` `sub_82990EC0 + 0x6A`.
- Next Windows boundary: confirm whether `sub_82990EC0` is another project-side wrapper self-call with a generated `__imp__sub_82990EC0`, then make one minimal fix and rerun the same build/smoke. If it is not a simple wrapper recursion, stop and trace the call path instead of guessing.

2026-06-17 Windows wrapper continuation 2:

- Confirmed `sub_82990EC0` had the same wrapper-recursion pattern. The project-side wrapper called `sub_82990EC0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82990EC0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.65.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82990EC0(...)` declaration, added `extern "C" void __imp__sub_82990EC0(...)`, and changed only the logging wrapper body to call `__imp__sub_82990EC0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-c1e3a3d8-b356-4682-bb04-452fffbd14be.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-5691770c-98e6-4d35-acbe-c6dcd890c50e.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_82990EC0`. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, and `sub_8218BE28 #500`. Latest repeated frame is RVA `0x78652`; subtract the PE/map `0x1000` delta to map offset `0x77652`, which falls in `imports.cpp.obj` `sub_827EEB48 + 0x202`.
- Next Windows boundary: inspect `sub_827EEB48` before editing. Because the repeated offset is not the previous `+0x6A` wrapper-entry pattern, confirm whether this is direct wrapper recursion, an internal recursive branch, or another call-path issue. Use IDA MCP if source/generated/map evidence is insufficient.

2026-06-17 Windows wrapper continuation 3:

- Confirmed `sub_827EEB48 + 0x202` was still the same wrapper-recursion class. The project-side wrapper called `sub_827EEB48(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_827EEB48)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.53.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_827EEB48(...)` declaration, added `extern "C" void __imp__sub_827EEB48(...)`, and changed only the logging wrapper body to call `__imp__sub_827EEB48(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-35d81ee6-1ada-4a47-b28c-4600043a71b0.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-46161e16-51f0-418c-8176-e3876f3e3ca5.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_827EEB48`. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, and `sub_8218BE28 #500`. Latest repeated frame is RVA `0x78B22`; subtract the PE/map `0x1000` delta to map offset `0x77B22`, which falls in `imports.cpp.obj` `sub_82197338 + 0x202`.
- Next Windows boundary: inspect `sub_82197338`; likely another project-side logging wrapper, but confirm generated `__imp__sub_82197338` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 4:

- Confirmed `sub_82197338 + 0x202` was another wrapper-recursion frame. The project-side wrapper called `sub_82197338(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82197338)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.3.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82197338(...)` declaration, added `extern "C" void __imp__sub_82197338(...)`, and changed only the logging wrapper body to call `__imp__sub_82197338(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-de26f4c7-378b-4660-b0c5-6c9bd3ad534d.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-8dee053b-a2d6-4af9-86d5-180f575c7cf3.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_82197338`. Smoke reached graphics backend attempts, MMIO/VBlank startup, `sub_8284F880` exit, `sub_8218BE28 #500`, and a separate guest-thread trace beginning at `0x829B08E0` / `sub_829A7960`. Latest repeated frame is RVA `0x78FF2`; subtract the PE/map `0x1000` delta to map offset `0x77FF2`, which falls in `imports.cpp.obj` `sub_821915F8 + 0x202`.
- Next Windows boundary: inspect `sub_821915F8`; likely another project-side logging wrapper, but confirm generated `__imp__sub_821915F8` and the exact call pattern before editing. Do not interpret the guest-thread trace as playability; the runtime still hits stack overflow.

2026-06-17 Windows wrapper continuation 5:

- Confirmed `sub_821915F8 + 0x202` was another wrapper-recursion frame. The project-side wrapper called `sub_821915F8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_821915F8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.3.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_821915F8(...)` declaration, added `extern "C" void __imp__sub_821915F8(...)`, and changed only the logging wrapper body to call `__imp__sub_821915F8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-72a04361-ee4c-4cdf-bb60-6bdeccf44899.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-43ca8b99-ed73-4e4f-a3f2-6c82874d2329.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_821915F8`. Smoke reached graphics backend attempts, MMIO/VBlank startup, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x62982`; subtract the PE/map `0x1000` delta to map offset `0x61982`, which falls in `imports.cpp.obj` `sub_82120EE8 + 0x1F2`.
- Next Windows boundary: inspect `sub_82120EE8 + 0x1F2`. This is outside the adjacent `sub_8218C600` logging-wrapper chain; confirm whether it is another wrapper self-call, an internal recursive branch, or a deeper runtime/memory allocation loop before editing. Use IDA MCP if source/generated/map evidence is insufficient.

2026-06-17 Windows wrapper continuation 6:

- Confirmed `sub_82120EE8 + 0x1F2` was another wrapper-recursion frame. The project-side wrapper called `sub_82120EE8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82120EE8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.0.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82120EE8(...)` declaration in the shared init trace declaration block, added `extern "C" void __imp__sub_82120EE8(...)`, and changed only the wrapper body to call `__imp__sub_82120EE8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-d9cfb870-73ff-4e0f-9e50-6301c42e2413.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-c5cd2249-fc4a-467c-84fd-8395fb1be688.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_82120EE8`. Smoke reached graphics backend attempts, MMIO/VBlank startup, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x62C78`; subtract the PE/map `0x1000` delta to map offset `0x61C78`, which falls in `imports.cpp.obj` `sub_821207B0 + 0x38`.
- Next Windows boundary: inspect `sub_821207B0 + 0x38`; do not assume it is the same wrapper-recursion pattern because this function has custom resource-manager initialization logic nearby. Confirm exact source/generator behavior before editing.

2026-06-17 Windows wrapper continuation 7:

- Confirmed `sub_821207B0 + 0x38` was another wrapper-recursion frame, despite having custom resource-manager post-initialization logic. The project-side wrapper called `sub_821207B0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_821207B0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.0.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_821207B0(...)` declaration, added `extern "C" void __imp__sub_821207B0(...)`, changed only the original-initialization call to `__imp__sub_821207B0(ctx, base)`, and preserved the wrapper's resource-manager field writes.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-7c5c3352-9441-4bc8-a15b-cc864187675c.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-957e8e5a-edcb-4a37-8e03-edcdc5199028.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_821207B0`. Smoke reached graphics backend attempts, MMIO/VBlank startup, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. The exception stack now includes `fmt` formatting frames (`main.cpp.obj`) followed by repeated RVA `0x67592`; subtract the PE/map `0x1000` delta to map offset `0x66592`, which falls in `imports.cpp.obj` `sub_82673718 + 0x1F2`.
- Next Windows boundary: inspect `sub_82673718 + 0x1F2`. The leading `fmt` frames are likely logging overhead; focus on whether `sub_82673718` is another project-side wrapper self-call before considering logging changes.

2026-06-17 Windows wrapper continuation 8:

- Confirmed `sub_82673718 + 0x1F2` was another wrapper-recursion frame. The project-side wrapper called `sub_82673718(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82673718)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.38.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82673718(...)` declaration, added `extern "C" void __imp__sub_82673718(...)`, and changed only the wrapper body to call `__imp__sub_82673718(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-9009e2af-c248-47c3-96a7-030568aadb69.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-c1a2a1da-cd3e-49e7-ba9f-d0b212ca5ce3.log`
- Fresh smoke result: process still exits through `0xC00000FD` stack overflow, but the repeated frame moved past `sub_82673718`. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x67A42`; subtract the PE/map `0x1000` delta to map offset `0x66A42`, which falls in `imports.cpp.obj` `sub_8297B8C0 + 0x1F2`.
- Next Windows boundary: inspect `sub_8297B8C0`; likely another project-side trace wrapper inside `sub_82673718`, but confirm generated `__imp__sub_8297B8C0` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 9:

- Confirmed `sub_8297B8C0 + 0x1F2` was another wrapper-recursion frame. The project-side wrapper called `sub_8297B8C0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8297B8C0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8297B8C0(...)` declaration, added `extern "C" void __imp__sub_8297B8C0(...)`, and changed only the wrapper body to call `__imp__sub_8297B8C0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-f1d852c6-1376-4e7e-af62-a63d97f1caaf.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-91c8687c-dc8b-4470-8f4c-c37b20f044bc.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. The repeated frame moved past `sub_8297B8C0`. Smoke reached graphics backend attempts, MMIO/VBlank startup, GPU MMIO writes, VBlank tick `#1`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x67D6A`; subtract the PE/map `0x1000` delta to map offset `0x66D6A`, which falls in `imports.cpp.obj` `sub_829735C8 + 0x6A`.
- Next Windows boundary: inspect `sub_829735C8`; likely another project-side trace wrapper in the same `sub_82673718` child-call block, but confirm generated `__imp__sub_829735C8` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 10:

- Confirmed `sub_829735C8 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_829735C8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_829735C8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_829735C8(...)` declaration, added `extern "C" void __imp__sub_829735C8(...)`, and changed only the wrapper body to call `__imp__sub_829735C8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-18c7562c-79c5-422f-905c-57db8d888d74.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-9e73dd53-1137-4bc3-aebf-14b9adbd9c8c.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. The repeated frame moved past `sub_829735C8`. Smoke reached graphics backend attempts, MMIO/VBlank startup, GPU MMIO writes, VBlank tick `#1`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x67E3A`; subtract the PE/map `0x1000` delta to map offset `0x66E3A`, which falls in `imports.cpp.obj` `sub_829C52F0 + 0x6A`.
- Next Windows boundary: inspect `sub_829C52F0`; it is a trace wrapper under `sub_829735C8`, but confirm generated `__imp__sub_829C52F0` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 11:

- Confirmed `sub_829C52F0 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_829C52F0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_829C52F0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.67.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_829C52F0(...)` declaration, added `extern "C" void __imp__sub_829C52F0(...)`, and changed only the wrapper body to call `__imp__sub_829C52F0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-559307f2-e107-4b71-84f9-31d0574797a1.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-41cab9ea-a8e8-459f-b150-c03b736f42dc.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. The repeated frame moved past `sub_829C52F0`. Smoke reached graphics backend attempts, MMIO/VBlank startup, GPU MMIO writes, VBlank tick `#2`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x67F0A`; subtract the PE/map `0x1000` delta to map offset `0x66F0A`, which falls in `imports.cpp.obj` `sub_829A0EA8 + 0x6A`.
- Next Windows boundary: inspect `sub_829A0EA8`; it is a trace wrapper under `sub_829735C8`, but confirm generated `__imp__sub_829A0EA8` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 12:

- Confirmed `sub_829A0EA8 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_829A0EA8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_829A0EA8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.66.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_829A0EA8(...)` declaration, added `extern "C" void __imp__sub_829A0EA8(...)`, and changed only the wrapper body to call `__imp__sub_829A0EA8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-691fefe1-3070-4034-9e01-aaed7d94113d.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-a463466f-95c9-45a5-8c9c-4cdf0ef43017.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. The repeated frame moved past `sub_829A0EA8`. Smoke reached graphics backend attempts, MMIO/VBlank startup, GPU MMIO writes, VBlank tick `#1`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x67FDA`; subtract the PE/map `0x1000` delta to map offset `0x66FDA`, which falls in `imports.cpp.obj` `sub_8296D468 + 0x6A`.
- Next Windows boundary: inspect `sub_8296D468`; it is a trace wrapper under `sub_829735C8`, but confirm generated `__imp__sub_8296D468` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 13:

- Confirmed `sub_8296D468 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_8296D468(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296D468)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296D468(...)` declaration, added `extern "C" void __imp__sub_8296D468(...)`, and changed only the wrapper body to call `__imp__sub_8296D468(ctx, base)`.
- Fresh ReXGlue CLI check:
  `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest\tools\rexglue-sdk-0.8.1.32-dev.gf22cd9d-win-amd64\bin\rexglue.exe --version`
  returned `0.8.1.32-dev.gf22cd9d`; `rexglue codegen --help` confirmed codegen is available.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-f85cf8bc-8f07-4cd7-b877-6f1e3492bdd2.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-449aba25-8d7b-4439-97ff-c37e7a0acad3.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. The repeated frame moved past `sub_8296D468`. Smoke reached graphics backend attempts, MMIO/VBlank startup, GPU MMIO writes, VBlank tick `#1`, `sub_8218BE28 #793/#794`, and multiple guest-thread traces entering/exiting `sub_829A7960`. Latest repeated frame is RVA `0x680AA`; subtract the PE/map `0x1000` delta to map offset `0x670AA`, which falls in `imports.cpp.obj` `sub_82974F90 + 0x6A`.
- Next Windows boundary: inspect `sub_82974F90`; it is adjacent to the current `sub_82673718` trace wrapper block, but confirm generated `__imp__sub_82974F90` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 14:

- Confirmed `sub_82974F90 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_82974F90(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82974F90)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82974F90(...)` declaration, added `extern "C" void __imp__sub_82974F90(...)`, and changed only the audio stream registration wrapper body to call `__imp__sub_82974F90(ctx, base)` after the `sub_829735C8` init path.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-06f7ade3-a7bb-4912-9d5b-8bd0c7e2a880.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-d776af90-bef8-412f-ac01-7bc063413500.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, and multiple guest-thread traces. The first exception address landed in external/system code, but the repeated repo frame is RVA `0x68302`; subtract the PE/map `0x1000` delta to map offset `0x67302`, which falls in `imports.cpp.obj` `sub_82670660 + 0x1F2`.
- Next Windows boundary: inspect `sub_82670660`; because the repeated offset is not the usual `+0x6A` wrapper-entry pattern, confirm whether this is direct wrapper recursion, an internal recursive branch, or a different call-path issue before editing. Use IDA MCP if source/generated/map evidence is insufficient.

2026-06-17 Windows wrapper continuation 15:

- Confirmed `sub_82670660 + 0x1F2` was direct wrapper recursion despite the non-`+0x6A` offset. The project-side wrapper called `sub_82670660(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82670660)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.38.cpp`. The old runtime backup also had this wrapper calling `__imp__sub_82670660`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82670660(...)` declaration, added `extern "C" void __imp__sub_82670660(...)`, and changed only the init trace wrapper body to call `__imp__sub_82670660(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-5378b713-2167-4421-b38d-0c63a5126636.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-0c238cf4-2716-4d76-9d85-0a7c88e47761.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, and multiple guest-thread traces. Latest repeated frame is RVA `0x687B2`; subtract the PE/map `0x1000` delta to map offset `0x677B2`, which falls in `imports.cpp.obj` `sub_82976570 + 0x1F2`.
- Next Windows boundary: inspect `sub_82976570`; it is adjacent in the same init trace wrapper block, but confirm generated `__imp__sub_82976570` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 16:

- Confirmed `sub_82976570 + 0x1F2` was another direct wrapper-recursion frame. The project-side wrapper called `sub_82976570(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82976570)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82976570(...)` declaration, added `extern "C" void __imp__sub_82976570(...)`, and changed only the init trace wrapper body to call `__imp__sub_82976570(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-6988072e-bbbc-47ba-8bfe-9a19b1a4f1ee.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-ceef6544-e03e-4d69-95d1-9e0f95dfbefc.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, and multiple guest-thread traces. Latest repeated frame is RVA `0x68C62`; subtract the PE/map `0x1000` delta to map offset `0x67C62`, which falls in `imports.cpp.obj` `sub_8297AD60 + 0x1F2`.
- Next Windows boundary: inspect `sub_8297AD60`; it is adjacent in the same init trace wrapper block, but confirm generated `__imp__sub_8297AD60` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 17:

- Confirmed `sub_8297AD60 + 0x1F2` was another direct wrapper-recursion frame. The project-side wrapper called `sub_8297AD60(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8297AD60)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8297AD60(...)` declaration, added `extern "C" void __imp__sub_8297AD60(...)`, and changed only the init trace wrapper body to call `__imp__sub_8297AD60(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-e3bf33c2-1af2-4afc-8668-193b8f2b2f68.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-bc7e5ce7-4dba-4add-8ff6-8d1c8bcf5576.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, and multiple guest-thread traces. Latest repeated frame is RVA `0x6916E`; subtract the PE/map `0x1000` delta to map offset `0x6816E`, which falls in `imports.cpp.obj` `sub_8297B260 + 0x24E`.
- Next Windows boundary: inspect `sub_8297B260`; it has a larger project-side wrapper than the simple init trace wrappers, so confirm whether the repeated frame is wrapper recursion, an intentional worker-thread loop, or another call-path issue before editing.

2026-06-17 Windows wrapper continuation 18:

- Confirmed `sub_8297B260 + 0x24E` was still direct wrapper recursion. The project-side audio worker wrapper called `sub_8297B260(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8297B260)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8297B260(...)` declaration, added `extern "C" void __imp__sub_8297B260(...)`, and changed only the audio worker wrapper body to call `__imp__sub_8297B260(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-3e0c79ff-d3fd-402a-afaf-5fb0d59155a9.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-ecfc2f79-1f07-4f0b-9b8e-3b4bb475d65e.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, and multiple guest-thread traces. Latest repeated frame is RVA `0x69DB4`; subtract the PE/map `0x1000` delta to map offset `0x68DB4`, which falls in `imports.cpp.obj` `sub_8298E810 + 0x204`.
- Next Windows boundary: inspect `sub_8298E810`; the smoke log tags it as `AUDIO_WORKER`, so confirm whether it is another wrapper recursion or a real worker-thread recursion before editing.

2026-06-17 Windows wrapper continuation 19:

- Confirmed `sub_8298E810 + 0x204` was another wrapper-recursion frame in the audio worker wrapper. The project-side wrapper called `sub_8298E810(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8298E810)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.65.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8298E810(...)` declarations, added one `extern "C" void __imp__sub_8298E810(...)` declaration near the earlier forward declaration, and changed only the audio worker wrapper body to call `__imp__sub_8298E810(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-7d9f387d-87f5-4c6f-8113-924e745c0f89.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-9c5ebdaa-b743-4835-b569-b6c26fb449a3.log`
- Fresh smoke result: smoke returned process exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and one `Guest code returned` line after the audio worker wrapper moved. Latest repeated frame is RVA `0x69432`; subtract the PE/map `0x1000` delta to map offset `0x68432`, which falls in `imports.cpp.obj` `sub_82975608 + 0x1F2`.
- Next Windows boundary: inspect `sub_82975608`; it is in the same audio/init wrapper neighborhood, but confirm generated `__imp__sub_82975608` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 20:

- Confirmed `sub_82975608 + 0x1F2` was another direct wrapper-recursion frame. The project-side wrapper called `sub_82975608(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82975608)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82975608(...)` declaration, added `extern "C" void __imp__sub_82975608(...)`, and changed only the logging wrapper body to call `__imp__sub_82975608(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. The latest verification rerun reported `ninja: no work to do`; prior compile warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-e0455769-9e01-48ec-aabb-c6f6955e601a.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-9d316b52-b72e-434d-a63d-ee60ea6b833b.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x698E4`; subtract the PE/map `0x1000` delta to map offset `0x688E4`, which falls in `imports.cpp.obj` `sub_829748D0 + 0x1F4`.
- Next Windows boundary: inspect `sub_829748D0`; it is inside `sub_82975608`, but confirm generated `__imp__sub_829748D0` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 21:

- Confirmed `sub_829748D0 + 0x1F4` was another direct wrapper-recursion frame. The project-side wrapper called `sub_829748D0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_829748D0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_829748D0(...)` declaration, added `extern "C" void __imp__sub_829748D0(...)`, and changed only the logging wrapper body to call `__imp__sub_829748D0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-d247aa26-a45f-4a7e-a2bf-deca3c81e7e4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-54f307c9-e158-4ea0-9299-b6d53eac9157.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7AEDF`; subtract the PE/map `0x1000` delta to map offset `0x79EDF`, which falls in `imports.cpp.obj` `sub_82974FF8 + 0x21F`.
- Next Windows boundary: inspect `sub_82974FF8`; source and generated-code evidence already show a project-side trace wrapper with generated `__imp__sub_82974FF8`, but rerun a narrow static RED check before editing.

2026-06-17 Windows wrapper continuation 22:

- Confirmed `sub_82974FF8 + 0x21F` was another direct wrapper-recursion frame. The project-side wrapper called `sub_82974FF8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82974FF8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82974FF8(...)` declaration, added `extern "C" void __imp__sub_82974FF8(...)`, and changed only the logging wrapper body to call `__imp__sub_82974FF8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-ba039bf1-47ec-4e35-81c7-1f9075cebedd.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-9f3474d4-8bc1-4db6-b577-4fc6262b9c06.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#2`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7DB44`; subtract the PE/map `0x1000` delta to map offset `0x7CB44`, which falls in `imports.cpp.obj` `sub_8296BF88 + 0x1F4`.
- Next Windows boundary: inspect `sub_8296BF88`; it is an early trace wrapper under `sub_82974FF8`, but confirm generated `__imp__sub_8296BF88` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 23:

- Confirmed `sub_8296BF88 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_8296BF88(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296BF88)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296BF88(...)` declaration, added `extern "C" void __imp__sub_8296BF88(...)`, and changed only the logging wrapper body to call `__imp__sub_8296BF88(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-8bda0f13-f5ac-40f9-9818-629f45835bc1.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-8b7ff805-3a7c-4b9d-9a5d-d14a34deaf67.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7E004`; subtract the PE/map `0x1000` delta to map offset `0x7D004`, which falls in `imports.cpp.obj` `sub_8296BFB8 + 0x1F4`.
- Next Windows boundary: inspect `sub_8296BFB8`; it is adjacent to `sub_8296BF88` in the early trace wrapper block, but confirm generated `__imp__sub_8296BFB8` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 24:

- Confirmed `sub_8296BFB8 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_8296BFB8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296BFB8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296BFB8(...)` declaration, added `extern "C" void __imp__sub_8296BFB8(...)`, and changed only the logging wrapper body to call `__imp__sub_8296BFB8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-32ebcaab-934d-4313-bd93-74feffd19ea6.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-4ad4982c-82a1-4e58-8a10-a26656979f6e.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#3`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7E4C4`; subtract the PE/map `0x1000` delta to map offset `0x7D4C4`, which falls in `imports.cpp.obj` `sub_82213C48 + 0x1F4`.
- Next Windows boundary: inspect `sub_82213C48`; it is the next adjacent early trace wrapper, but confirm generated `__imp__sub_82213C48` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 25:

- Confirmed `sub_82213C48 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_82213C48(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82213C48)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.6.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82213C48(...)` declaration, added `extern "C" void __imp__sub_82213C48(...)`, and changed only the logging wrapper body to call `__imp__sub_82213C48(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-9eb11b95-adb1-40dd-852c-5a43b2efeb99.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-9a4f48f7-52e5-4453-9a4c-98ebed31c8f8.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7E984`; subtract the PE/map `0x1000` delta to map offset `0x7D984`, which falls in `imports.cpp.obj` `sub_8296C228 + 0x1F4`.
- Next Windows boundary: inspect `sub_8296C228`; it is the next adjacent early trace wrapper, but confirm generated `__imp__sub_8296C228` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 26:

- Confirmed `sub_8296C228 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_8296C228(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296C228)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296C228(...)` declaration, added `extern "C" void __imp__sub_8296C228(...)`, and changed only the logging wrapper body to call `__imp__sub_8296C228(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-24a5bcb0-b0a7-4c10-b63a-d3579e4dcba6.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-29290189-2846-4852-b5b1-d664e790d5cc.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7EE44`; subtract the PE/map `0x1000` delta to map offset `0x7DE44`, which falls in `imports.cpp.obj` `sub_82974530 + 0x1F4`.
- Next Windows boundary: inspect `sub_82974530`; it is the next adjacent early trace wrapper, but confirm generated `__imp__sub_82974530` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 27:

- Confirmed `sub_82974530 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_82974530(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82974530)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82974530(...)` declaration, added `extern "C" void __imp__sub_82974530(...)`, and changed only the logging wrapper body to call `__imp__sub_82974530(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-17ac2904-1c27-4eee-93cc-9197cfce4fa4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-737d7d6b-3ae9-427a-ab10-0b63bce8d662.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#2`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7F304`; subtract the PE/map `0x1000` delta to map offset `0x7E304`, which falls in `imports.cpp.obj` `sub_8296C238 + 0x1F4`.
- Next Windows boundary: inspect `sub_8296C238`; it is the next adjacent early trace wrapper, but confirm generated `__imp__sub_8296C238` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 28:

- Confirmed `sub_8296C238 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_8296C238(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296C238)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296C238(...)` declaration, added `extern "C" void __imp__sub_8296C238(...)`, and changed only the logging wrapper body to call `__imp__sub_8296C238(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-918b1038-30ae-471b-9322-cd306fcb69a9.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-1fadc5af-fa17-4b33-bfc8-cc2555190917.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#2`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7D684`; subtract the PE/map `0x1000` delta to map offset `0x7C684`, which falls in `imports.cpp.obj` `sub_82974500 + 0x1F4`.
- Next Windows boundary: inspect `sub_82974500`; this is still in the same early trace-wrapper cluster, but confirm generated `__imp__sub_82974500` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 29:

- Confirmed `sub_82974500 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_82974500(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82974500)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.64.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82974500(...)` declaration, added `extern "C" void __imp__sub_82974500(...)`, and changed only the logging wrapper body to call `__imp__sub_82974500(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-09d6b642-37fd-4054-92b5-1249727fec68.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-85311836-08a3-4fdf-b314-11d9d4ef8d23.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7CD14`; subtract the PE/map `0x1000` delta to map offset `0x7BD14`, which falls in `imports.cpp.obj` `sub_8296C378 + 0x1F4`.
- Next Windows boundary: inspect `sub_8296C378`; this is still in the same early trace-wrapper cluster, but confirm generated `__imp__sub_8296C378` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 30:

- Confirmed `sub_8296C378 + 0x1F4` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_8296C378(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8296C378)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.63.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8296C378(...)` declaration, added `extern "C" void __imp__sub_8296C378(...)`, and changed only the logging wrapper body to call `__imp__sub_8296C378(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-0a18fc97-3134-4d4f-87e9-843666961ca3.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-c31fe00f-a6f4-40a1-a605-6f39afd057c4.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached graphics backend attempts, MMIO/VBlank startup, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7D1D2`; subtract the PE/map `0x1000` delta to map offset `0x7C1D2`, which falls in `imports.cpp.obj` `sub_82763AB8 + 0x1F2`.
- Next Windows boundary: inspect `sub_82763AB8`; this is still in the same early trace-wrapper cluster, but confirm generated `__imp__sub_82763AB8` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 31:

- Confirmed `sub_82763AB8 + 0x1F2` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_82763AB8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82763AB8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.48.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82763AB8(...)` declaration, added `extern "C" void __imp__sub_82763AB8(...)`, and changed only the logging wrapper body to call `__imp__sub_82763AB8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-58d07924-d525-4236-9266-e11fce0365f7.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-50983a35-b1f0-4fa5-a3e6-1caabc8be51c.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached D3D12/Vulkan backend attempts, `[MMIO-BRIDGE]` writes, VBlank tick `#1`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x7B3DF`; subtract the PE/map `0x1000` delta to map offset `0x7A3DF`, which falls in `imports.cpp.obj` `sub_82671E40 + 0x21F`.
- Next Windows boundary: inspect `sub_82671E40`; this is still in the same early trace-wrapper cluster, but confirm generated `__imp__sub_82671E40` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 32:

- Confirmed `sub_82671E40 + 0x21F` was another direct wrapper-recursion frame. The project-side trace wrapper called `sub_82671E40(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82671E40)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.38.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: added `extern "C" void __imp__sub_82671E40(...)` immediately before the logging wrapper and changed only the wrapper body to call `__imp__sub_82671E40(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-312347fe-26da-4809-b92b-6ed5e1d1db8c.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-6eb1646b-07d8-4651-a5d1-6449ca58ef70.log`
- Fresh smoke result: process returned exit code `0`, but the log still captured `VEH exception code=0xC00000FD`, so the runtime is not stable yet. Smoke reached D3D12/Vulkan backend attempts, `[MMIO-BRIDGE]` writes, VBlank tick `#2`, `sub_8284F880` exit, `sub_8218BE28 #793/#794`, multiple guest-thread traces, and `Guest code returned`. Latest repeated frame is RVA `0x65704`; subtract the PE/map `0x1000` delta to map offset `0x64704`, which falls in `imports.cpp.obj` `sub_82269098 + 0x1F4`.
- Next Windows boundary: inspect `sub_82269098`; confirm generated `__imp__sub_82269098` and the exact call pattern before editing.

Current finding: a first full `Image::ParseImage()` attempt reached `default.xex` read success (`module bytes=11841536`) and did not return within the 240-second Ryujinx window. The subsequent Switch-local phase probe narrowed that broad stall to host-loader memory pressure: duplicate decrypted-buffer allocation can enter the GCC unwinder path, while in-place AES decryption completes and the next `0x11F0000` decompression output allocation fails cleanly.

Previous stage result: lightweight XEX metadata preflight passes in Ryujinx with the real staged layout. It reads `default.xex`, validates the XEX2 header bounds, logs security/file-format/resource/import metadata, and stops before `Image::ParseImage()`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.

Current stage rule: do not modify `tools/XenonRecomp` yet. First reproduce or narrow the parse behavior with local/read-only evidence, then add Switch-local audit instrumentation only if needed.

Local host-side profiling result: a read-only parser against `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)\default.xex` completed XEX key decrypt, image decrypt, basic decompression, and PE section scan in about `461 ms` on Windows. It reported three basic-compression blocks, `13` PE sections, `imageSize=0x11F0000`, `imageBase=0x82000000`, and entry `0x829A0860`. This suggests the 240-second Ryujinx stall is not simply that the source XEX is too large to parse; the next minimal Switch boundary should log equivalent phases inside the Switch audit path before calling full `Image::ParseImage()`.

Latest Switch result: the staged basic-decompression view avoids the `0x11F0000` output allocation, reads PE/import slices directly from the decrypted payload, completes the module preflight in Ryujinx, materializes the staged XEX image into the mapped guest image span, validates/initializes the XDBF resource wrapper, zeroes the collision range, initializes the stream struct, and zeroes worker globals. It finds `13` PE sections, `2` import libraries, `484` import descriptors, `0` missing thunk targets, copies `11829248` staged data bytes, zero-fills `6979584` bytes, validates XDBF `entries=67 freeTable=34`, verifies the side-effect writes, then stops before `GuestThread::Start()`.

Current boundary decision: the Switch pre-guest baseline now covers startup container, SD content path, VFS indexing/path resolution, XEX metadata/decrypt/staged PE/import scan, mapped guest image span, staged image materialization, and the known pre-guest loader side effects. The next decision is whether to stop Switch feature work here and pivot mainline effort to Windows runtime code, using Switch only for regression verification and small pre-guest fixes.

Identified side-effect ranges before editing:

- XDBF/resource wrapper source: `resourceOffset=0x83150000`, `resourceSize=0x0009BA69`; construct `g_xdbfWrapper` from translated guest memory and verify the XDBF header/table bounds.
- Collision zero: `0x82003880..0x82003900` (`0x80` bytes), matching the existing `LdrLoadModule()` workaround.
- Stream struct: `0x82003890..0x820038AC` (`0x1C` bytes / seven `be<uint32_t>` fields), all set to zero after the collision clear.
- Worker globals: `0x830F5000..0x830F8000` (`0x3000` bytes), zeroed so uninitialized worker handles read as null.

First guest-memory-enabled attempt result: with the full scanned image/function-table mapping retained, Ryujinx returned `0x0000D001` while mapping `image/function table guest=0x82000000 size=0x24A0000`. `Memory::base` stayed null, so the module preflight logged all planned `LdrLoadModule()` ranges but skipped touches. The next retry should explicitly set `LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE=0x11F0000` and `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS=ON` so this stage validates only the XEX image span and does not need generated function-table insertion.

2026-06-17 Windows wrapper continuation 33:

- Confirmed `sub_82269098 + 0x1F4` was another wrapper-recursion frame. The project-side wrapper called `sub_82269098(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82269098)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.8.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82269098(...)` declaration, added `extern "C" void __imp__sub_82269098(...)`, and changed only the logging wrapper body to call `__imp__sub_82269098(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-808d23ec-c740-40cc-ba37-1de09c87309b.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-450fb275-bb10-49dc-96b4-0edbaa6324e5.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow, so this is not a pass. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, `sub_829A7960` traces, VBlank tick `#1`, and `Guest code returned` before the stack overflow trace. Latest repeated frame is RVA `0x7FC92`; subtract the PE/map `0x1000` delta to map offset `0x7EC92`, which falls in `imports.cpp.obj` `sub_822054F8 + 0x1F2`.
- Next Windows boundary: inspect `sub_822054F8`; likely another project-side logging wrapper, but confirm generated `__imp__sub_822054F8` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 34:

- Confirmed `sub_822054F8 + 0x1F2` was another wrapper-recursion frame. The project-side wrapper called `sub_822054F8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_822054F8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.6.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: added `extern "C" void __imp__sub_822054F8(...)` and changed only the logging wrapper body to call `__imp__sub_822054F8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-52366154-ba10-40f9-93ca-28a238fc9e89.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-eac6e811-d092-435f-8345-b925ac523cbf.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, `sub_829A7960` traces, VBlank tick `#1`, and `Guest code returned` before the stack overflow trace. Latest repeated frame is RVA `0x81AD4`; subtract the PE/map `0x1000` delta to map offset `0x80AD4`, which falls in `imports.cpp.obj` `sub_821250B0 + 0x1F4`.
- Next Windows boundary: inspect `sub_821250B0`; likely another project-side logging wrapper, but confirm generated `__imp__sub_821250B0` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 35:

- Confirmed `sub_821250B0 + 0x1F4` was another wrapper-recursion frame. The project-side wrapper used a split `PPC_FUNC(sub_821250B0)` / `{` format and called `sub_821250B0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_821250B0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.0.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: added `extern "C" void __imp__sub_821250B0(...)` and changed only the logging wrapper body to call `__imp__sub_821250B0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-13f62a02-1404-40d3-98c2-a303fcfeb678.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-2337358e-e382-4c01-9eb9-ac0d1a705af3.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank tick `#1`, and two `Guest code returned` lines before the stack overflow trace. Latest repeated frame is RVA `0x81F94`; subtract the PE/map `0x1000` delta to map offset `0x80F94`, which falls in `imports.cpp.obj` `sub_82318F60 + 0x1F4`.
- Next Windows boundary: inspect `sub_82318F60`; likely another project-side logging wrapper, but confirm generated `__imp__sub_82318F60` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 36:

- Confirmed `sub_82318F60 + 0x1F4` was another wrapper-recursion frame. The project-side wrapper used the split `PPC_FUNC(sub_82318F60)` / `{` format and called `sub_82318F60(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82318F60)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.12.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: added `extern "C" void __imp__sub_82318F60(...)` and changed only the logging wrapper body to call `__imp__sub_82318F60(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-27829bfd-f953-4759-ade6-7cbf9ffb2a1f.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-eca54863-12e4-47c5-802b-7f9a4660e1af.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank tick `#1`, and `Guest code returned` before the stack overflow trace. Latest repeated frame is RVA `0x7F7CD`; subtract the PE/map `0x1000` delta to map offset `0x7E7CD`, which falls in `imports.cpp.obj` `sub_824C1338 + 0x1FD`.
- Next Windows boundary: inspect `sub_824C1338`; confirm whether it is direct wrapper recursion, an internal recursive branch, or a different call-path problem before editing.

2026-06-17 Windows wrapper continuation 37:

- Confirmed `sub_824C1338 + 0x1FD` was another wrapper-recursion frame. The project-side wrapper called `sub_824C1338(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_824C1338)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.24.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_824C1338(...)` declaration, added `extern "C" void __imp__sub_824C1338(...)`, and changed only the logging wrapper body to call `__imp__sub_824C1338(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-9d245045-3876-49ab-a46a-07584f5390d4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-435764c7-30be-4587-a903-8cbc2b911c3e.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank tick `#1`, and `Guest code returned` before the stack overflow trace. Latest repeated frame is RVA `0x66422`; subtract the PE/map `0x1000` delta to map offset `0x65422`, which falls in `imports.cpp.obj` `sub_821DE390 + 0x1F2`.
- Next Windows boundary: inspect `sub_821DE390`; likely another project-side logging wrapper, but confirm generated `__imp__sub_821DE390` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 38:

- Confirmed `sub_821DE390 + 0x1F2` was another wrapper-recursion frame. The project-side wrapper called `sub_821DE390(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_821DE390)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.5.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: added `extern "C" void __imp__sub_821DE390(...)` and changed only the logging wrapper body to call `__imp__sub_821DE390(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-9da96892-33b7-4d4c-a00f-4e1cfbbf5ad3.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-0a343359-bf91-4235-b8fb-24b21a977dc8.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank tick `#1`, and `Guest code returned` before the stack overflow trace. Latest repeated frame is RVA `0x6674A`; subtract the PE/map `0x1000` delta to map offset `0x6574A`, which falls in `imports.cpp.obj` `sub_82204770 + 0x6A`.
- Smoke execution note: the 60-second smoke wrapper had tool-level output issues after this fix, so this verification used a 15-second bounded `Start-Process` smoke and then cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Next Windows boundary: inspect `sub_82204770`; the `+0x6A` offset matches the common logging-wrapper entry pattern, but confirm generated `__imp__sub_82204770` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 39:

- Confirmed `sub_82204770 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_82204770(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82204770)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.5.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82204770(...)` declaration, added `extern "C" void __imp__sub_82204770(...)`, and changed only the logging wrapper body to call `__imp__sub_82204770(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-5ecc2001-09d0-4937-ac47-c33c62dbc0cb.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-12fca086-29f0-4a9c-9830-f6b165b021ba.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank ticks `#2` and `#3`, and two `Guest code returned` lines before the stack overflow trace. Latest repeated frame is RVA `0x6681A`; subtract the PE/map `0x1000` delta to map offset `0x6581A`, which falls in `imports.cpp.obj` `sub_82124EF0 + 0x6A`.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commit `a4f599a8` for continuation 38 could not be pushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_82124EF0`; the `+0x6A` offset matches the common logging-wrapper entry pattern, but confirm generated `__imp__sub_82124EF0` and the exact call pattern before editing.

2026-06-17 Windows wrapper continuation 40:

- Confirmed `sub_82124EF0 + 0x6A` was another wrapper-recursion frame. The project-side wrapper called `sub_82124EF0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82124EF0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.0.cpp`.
- Added a narrow static regression check before editing; the first broad check had a false positive because `__imp__sub_82124EF0` contains the public symbol name as a substring, then the line-exact check passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82124EF0(...)` declaration, added `extern "C" void __imp__sub_82124EF0(...)`, and changed only the logging wrapper body to call `__imp__sub_82124EF0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-323fa8ea-3c32-468d-8e02-a382762a26f8.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-78cf480c-7625-4c2e-b1e1-fbae4aa9d409.log`
- Fresh smoke result: the 15-second bounded smoke reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank ticks through `#3`, and multiple `Guest code returned` lines, then logged `0xC00000FD` stack overflow before the harness killed the still-running process at the 15-second limit.
- Latest repeated frame is RVA `0x668F6`; the map places `sub_82205438` at `0x140066880`, and objdump shows `0x1400668F1` is a call back to `0x140066880`, with return address `0x1400668F6`. This confirms the next stack frame is `sub_82205438` wrapper recursion, not `sub_82124EF0` generated implementation recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_82205438`; confirm generated `__imp__sub_82205438` and the exact wrapper call pattern before editing.

2026-06-17 Windows wrapper continuation 41:

- Confirmed `sub_82205438` was another wrapper-recursion frame. The project-side wrapper called `sub_82205438(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82205438)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.6.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82205438(...)` declaration, added `extern "C" void __imp__sub_82205438(...)`, and changed only the logging wrapper body to call `__imp__sub_82205438(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-498307f7-b22c-4638-942b-150b0b6ae5c4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-35b00b6d-1b5a-4bc6-bf5c-869d6050b579.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, multiple `sub_829A7960` traces, VBlank ticks through `#3`, and `Guest code returned` before the stack overflow trace.
- Latest repeated frame is RVA `0x66B9A`; the map places `sub_827DB2A8` at `0x140066B30`, and objdump shows `0x140066B95` is a call back to `0x140066B30`, with return address `0x140066B9A`. This confirms the next stack frame is `sub_827DB2A8` wrapper recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_827DB2A8`; confirm generated `__imp__sub_827DB2A8` and the exact wrapper call pattern before editing.

2026-06-17 Windows wrapper continuation 42:

- Confirmed `sub_827DB2A8` was another wrapper-recursion frame. The project-side wrapper called `sub_827DB2A8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_827DB2A8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.52.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_827DB2A8(...)` declaration, added `extern "C" void __imp__sub_827DB2A8(...)`, and changed only the logging wrapper body to call `__imp__sub_827DB2A8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-1398fe47-b07f-4246-ba3f-44224d739d3e.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-30ff77d9-4e78-40dd-b40e-6d7c97c6228c.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached MMIO bridge writes, graphics backend attempts, guest thread startup, VBlank ticks through `#3`, `Guest code returned`, and a real VFS path resolution attempt for `platform:/textures/fonts` before the stack overflow trace.
- VFS evidence: `[GTA::FileResolve] #1 path='platform:/textures/fonts'`, then `[VFS] NOT FOUND: 'platform:/textures/fonts' (stripped='textures/fonts')`. This is useful progress because the startup path is now reaching project-side content lookup before the next wrapper recursion.
- Latest repeated frame is RVA `0x67166`; the map places `sub_82205390` at `0x1400670F0`, and objdump shows `0x140067161` is a call back to `0x1400670F0`, with return address `0x140067166`. This confirms the next stack frame is `sub_82205390` wrapper recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_82205390`; confirm generated `__imp__sub_82205390` and the exact wrapper call pattern before editing.

2026-06-17 Windows wrapper continuation 43:

- Confirmed `sub_82205390` was another wrapper-recursion frame. The project-side wrapper called `sub_82205390(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_82205390)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.6.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_82205390(...)` declaration, added `extern "C" void __imp__sub_82205390(...)`, and changed only the logging wrapper body to call `__imp__sub_82205390(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-cb55f287-32cd-40c4-86fd-11463853d9b4.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-090c3259-a57d-4371-bb82-7eed04127fed.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run again reached MMIO bridge writes, graphics backend attempts, guest thread startup, VBlank ticks, `Guest code returned`, and VFS lookup for `platform:/textures/fonts` before the stack overflow trace.
- Latest repeated frame is RVA `0x66E1D`; the map places `sub_827EDED0` at `0x140066C00`, and objdump shows `0x140066E18` is a call back to `0x140066C00`, with return address `0x140066E1D`. This confirms the next stack frame is `sub_827EDED0` wrapper recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_827EDED0`; confirm generated `__imp__sub_827EDED0` and the exact wrapper call pattern before editing.

2026-06-17 Windows wrapper continuation 44:

- Confirmed `sub_827EDED0` was another wrapper-recursion frame. The project-side wrapper called `sub_827EDED0(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_827EDED0)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.53.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_827EDED0(...)` declaration, added `extern "C" void __imp__sub_827EDED0(...)`, and changed only the logging wrapper body to call `__imp__sub_827EDED0(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-9d09c338-5d61-4efd-8582-a564adb94de8.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-d3a33fd2-1d1b-49c2-aeae-4b28836ea1e5.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run reached the earlier MMIO/VBlank/guest-thread evidence and advanced VFS lookup from `platform:/textures/fonts` to `platform:/textures/buttons_360` before the stack overflow trace.
- Latest repeated frame is RVA `0x80142`; the map places `sub_8221F8A8` at `0x14007FF50`, and objdump shows `0x14008013D` is a call back to `0x14007FF50`, with return address `0x140080142`. This confirms the next stack frame is `sub_8221F8A8` wrapper recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_8221F8A8`; confirm generated `__imp__sub_8221F8A8` and the exact wrapper call pattern before editing.

## Explicit Non-Goals

- Do not claim or imply the Switch build is playable.
- Do not enter guest/gameplay execution until guest memory/page backing has a documented audit boundary that permits it.
- Do not rewrite the generated runtime, renderer, audio stack, installer, or guest-memory architecture in this stage.
- Do not modify thirdparty submodules unless the current stage explicitly proves a submodule change is required and records why first.
- Do not build `LibertyRecompNro` unless testing Homebrew Menu icon/name/NRO launch behavior.
- Do not revert unrelated dirty worktree changes.
- Do not use files under `D:\Nintendo` as linked headers/libraries or copy official SDK source into the libnx target.

## Dirty Worktree Policy

Can touch in the current stage:

- `LibertyRecomp/kernel/imports.cpp`
- `LibertyRecomp/kernel/memory.cpp`
- `LibertyRecomp/kernel/memory.h`
- `LibertyRecomp/main.cpp`
- Switch audit CMake options in `CMakeLists.txt` only if a new audit stop requires it.
- Switch audit docs under `docs/switch-audit/`
- Old workspace copies under `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns` when syncing updated switch-audit docs.

Do not touch unless a later stage explicitly scopes it:

- `docs/backups/github-backup-20260615-022231/submodule-diffs/*.patch`
- `thirdparty/concurrentqueue`
- other dirty `thirdparty/*` submodules currently reported by `git status`
- `thirdparty/implot`
- `thirdparty/plume`
- `tools/XenonRecomp`
- other dirty `tools/*` submodules currently reported by `git status`
- `.planning/`
- `docs/dev/`

## Next Small Tasks

1. Inspect the Windows stack overflow now landing in `sub_8221F8A8` wrapper recursion.
   Completion standard: determine whether it is direct wrapper recursion, an internal recursive branch, or a different call-path problem before editing; use IDA MCP if source/generated/map evidence is insufficient.

2. Keep the ReXGlue MMIO/VBlank bootstrap evidence current.
   Completion standard: each smoke records whether `[MMIO-BRIDGE]`, VBlank callback/thread startup, GPU MMIO writes, and the latest repeated stack frame are present.

3. Keep Switch frozen as a regression baseline.
   Completion standard: do not build or modify `LibertyRecompNro`/`LibertyRecompExeFs` unless a Windows-side change needs a Switch regression check or a Switch-only blocker is explicitly scoped.

4. Document each Windows blocker movement.
   Completion standard: update this guide with build result, smoke log paths, latest repeated RVA/map offset, and next exact symbol before committing.

5. Avoid parallel code conflicts.
   Completion standard: no other conversation edits `LibertyRecomp/kernel/imports.cpp`, `LibertyRecomp/kernel/memory.cpp`, `LibertyRecomp/kernel/memory.h`, or `LibertyRecomp/main.cpp` while this Windows startup stage is active.

6. Document, commit, and push each meaningful stage.
   Completion standard: update this guide first, sync updated switch-audit docs to `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns`, commit only scoped files, and push current `codex/switch-audit-20260615` branch using `D:\Git\cmd\git.exe`.

## Current Stage Verification

- Fresh default baseline Build ID:
  `a535efd5cac29d1968d0afb08d5de82e2033955f`
- Fresh default baseline Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-36-33.log`
- Full `Image::ParseImage()` attempt Build ID:
  `3d5ab2fd65331f2d2db15c89b79ad1ca15b4dbf6`
- Full `Image::ParseImage()` attempt Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-40-54.log`
- Lightweight module metadata preflight Build ID:
  `92fe5a27c54121fc0dd73f3bcf81bab270eef09f`
- Lightweight module metadata preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-47-50.log`
- Lightweight module metadata preflight key line:
  `xex moduleFlags=0x00000001 headerSize=0x3000 security=0x90 optHeaders=15 imageSize=0x11F0000 load=0x82000000 imageBase=0x82000000 entry=0x829A0860 resource=0x83150000+0x0009BA69 fileFormat enc=1 comp=1 imports=2 pages=287`
- Restored default ExeFS Build ID:
  `06726e9a15c3c4a05df9ed3e35d93961ed5cc581`
- Restored default ExeFS Ryujinx smoke log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-49-32.log`
- Restored default result:
  all Switch audit stops OFF, guest-memory audit OFF, no `TEXTREL`, Ryujinx reaches missing `sdmc:/switch/LibertyRecomp/game/default.xex`, PTC restored, and Ryujinx SD `game` directory is empty.
- Full-parse phase probe Build ID:
  `0302534e936f6c49b448b878e2ba77bb2e51257d`
- Full-parse phase probe Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-16-23.log`
- Full-parse phase probe result:
  in-place AES-CBC decrypted the full payload through offset `0xB48000`; basic decompression reported `blocks=3`, `compressedBytes=11829248`, `expectedImageSize=0x11F0000`, then `basic decompression allocation failed`; the audit stopped before `Image::ParseImage`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.
- Staged decompression/import preflight Build ID:
  `2d924a12e48519c4c356a64419f4f41b69cf3c6d`
- Staged decompression/import preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-37-39.log`
- Staged decompression/import preflight result:
  in-place AES-CBC decrypted the full payload, staged basic-decompression view reported `expectedImageSize=0x11F0000`, PE scan reported `13` sections, import scan reported `2` libraries, `484` descriptors, and `0` missing thunk targets; the audit stopped before `Image::ParseImage`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.
- Restored default ExeFS Build ID after full-parse probe:
  `7b8900742ad10b66dc050441bed9b136890e1d25`
- Restored default ExeFS Ryujinx smoke log after full-parse probe:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-40-34.log`
- Fresh default baseline before guest-range probe:
  `cd76cff9c8760ec90f83b5933f812e0d42fbe60e`
- Fresh default baseline Ryujinx log before guest-range probe:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-51-09.log`
- Full scanned image/function-table guest-range attempt Build ID:
  `76b9796c512cae9b6b0a62b335188dc8711e632b`
- Full scanned image/function-table guest-range attempt Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-58-04.log`
- Full scanned image/function-table result:
  sparse memory mapped low guest heap, XMA I/O, and `0xF00000` physical heap, then failed at `image/function table guest=0x82000000 size=0x24A0000` with `0x0000D001`; `Memory::base` stayed null, so the module preflight logged planned ranges but skipped touches.
- Narrow module-image guest-range probe Build ID:
  `41db7ed33ecfe3d7b2060bb365900742329c2e83`
- Narrow module-image guest-range probe Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_00-11-49.log`
- Narrow module-image guest-range result:
  with `LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE=0x11F0000` and `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS=ON`, sparse memory mapped `0x82000000..0x831F0000`, `Memory::base=0x80000000`, and first/last-byte save/restore touches succeeded for image copy, resource, collision zero, stream struct, and worker globals. The audit stopped before `Image::ParseImage`, real `LdrLoadModule()` writes, and `GuestThread::Start()`.
- Restored default ExeFS Build ID after guest-range probe:
  `eff804d37589df5723b7f334f612047e5215dabe`
- Restored default ExeFS Ryujinx smoke log after guest-range probe:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_00-15-18.log`
- Staged XEX image materialization preflight Build ID:
  `e2d0ffd8cd633884d4e80e2d71ba0edd9cfaa7c5`
- Staged XEX image materialization Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-45-38.log`
- Staged XEX image materialization result:
  with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT=ON`, `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT=ON`, `LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE=0x11F0000`, and `LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS=ON`, sparse memory mapped `0x82000000..0x831F0000`, staged decrypt/PE/import scan completed, the staged image view was published, first/last-byte range probes succeeded, and materialization completed with `blocks=3 dataBytes=11829248 zeroBytes=6979584 imageSize=0x11F0000`. The audit stopped before XDBF setup, `LdrLoadModule()` side effects, and `GuestThread::Start()`.
- Restored default ExeFS Build ID after staged materialization:
  `443a3f6ce162fd464fe85728d4ba3b93a01f5dd9`
- Restored default ExeFS Ryujinx smoke log after staged materialization:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-50-06.log`
- Restored default result after staged materialization:
  all Switch audit stops OFF, guest-memory audit OFF, image-table override empty, no `TEXTREL`, Ryujinx reaches missing `sdmc:/switch/LibertyRecomp/game/default.xex`, no module-load preflight is entered, Ryujinx PTC stayed `false`, and Ryujinx SD `game` directory is empty.
- Loader side-effect audit preflight Build ID:
  `3f202d032852efd899e688cf932c0c236c2d6b24`
- Loader side-effect audit Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-58-14.log`
- Loader side-effect audit result:
  with the same narrow sparse-memory profile, staged image materialization completed, XDBF resource validation reported `entries=67 freeTable=34 resourceSize=0x0009BA69`, `g_xdbfWrapper` was initialized from `0x83150000..0x831EBA69`, collision zero `0x82003880..0x82003900` was verified, stream struct `0x82003890..0x820038AC` was initialized as seven zero dwords, worker globals `0x830F5000..0x830F8000` were zeroed, and the audit stopped before `GuestThread::Start()`.
- Restored default ExeFS Build ID after loader side-effect audit:
  `49a1b018938f7a716d73a11ed9ff2256a19d4af6`
- Restored default ExeFS Ryujinx smoke log after loader side-effect audit:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_04-02-06.log`
- Restored default result after loader side-effect audit:
  all Switch audit stops OFF, guest-memory audit OFF, image-table override empty, no `TEXTREL`, Ryujinx reaches missing `sdmc:/switch/LibertyRecomp/game/default.xex`, no module-load preflight is entered, and Ryujinx SD `game` directory is empty.

## Latest Verified Baselines

- Branch: `codex/switch-audit-20260615`
- Latest pushed commit before staged materialization: `32f66851 Audit Switch module image guest ranges`
- Earlier pushed commit: `c367be24 Audit Switch staged XEX import preflight`
- Latest pushed commit before staged import preflight: `228a619b Audit Switch XEX parse memory boundary`
- Earlier pushed commit: `16d1358e Harden Switch VFS recursive index audit`
- Latest pushed commit with module metadata preflight: `f76e7738 Add Switch module metadata preflight audit`
- Latest recursive VFS preflight Build ID: `9270023bb67df2533a5686d991efa7ad3e9933ed`
- Latest recursive VFS preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_19-00-43.log`
- Latest default ExeFS Build ID: `b6e5b56b55b38e03994464386eda9f99ab6cfd19`
- Latest default ExeFS Ryujinx smoke log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_19-03-27.log`
- Latest fresh default ExeFS Build ID:
  `a535efd5cac29d1968d0afb08d5de82e2033955f`
- Latest fresh default ExeFS Ryujinx smoke log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-36-33.log`
- First module parse attempt Build ID:
  `3d5ab2fd65331f2d2db15c89b79ad1ca15b4dbf6`
- First module parse attempt Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-40-54.log`
- First module parse attempt result:
  reached `Switch module-load preflight audit: parsing module image.` after reading `11841536` bytes, then did not reach a summary before the 240-second test window ended.
- Latest module metadata preflight Build ID:
  `92fe5a27c54121fc0dd73f3bcf81bab270eef09f`
- Latest module metadata preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-47-50.log`
- Latest restored default ExeFS Build ID:
  `06726e9a15c3c4a05df9ed3e35d93961ed5cc581`
- Latest restored default ExeFS Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_22-49-32.log`
- Latest local host-side XEX profiling:
  `default.xex` key decrypt/image decrypt/basic decompression/PE scan completed in about `461 ms`; no repo files were modified by the profiling command.
- Latest Switch full-parse phase probe Build ID:
  `0302534e936f6c49b448b878e2ba77bb2e51257d`
- Latest Switch full-parse phase probe Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-16-23.log`
- Latest Switch staged decompression/import preflight Build ID:
  `2d924a12e48519c4c356a64419f4f41b69cf3c6d`
- Latest Switch staged decompression/import preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-37-39.log`
- Latest restored default ExeFS Build ID:
  `eff804d37589df5723b7f334f612047e5215dabe`
- Latest restored default ExeFS Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_00-15-18.log`
- Latest Switch module-image guest-range preflight Build ID:
  `41db7ed33ecfe3d7b2060bb365900742329c2e83`
- Latest Switch module-image guest-range preflight Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_00-11-49.log`
- Latest Switch staged XEX image materialization Build ID:
  `e2d0ffd8cd633884d4e80e2d71ba0edd9cfaa7c5`
- Latest Switch staged XEX image materialization Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-45-38.log`
- Latest restored default ExeFS Build ID:
  `443a3f6ce162fd464fe85728d4ba3b93a01f5dd9`
- Latest restored default ExeFS Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-50-06.log`
- Latest Switch loader side-effect audit Build ID:
  `3f202d032852efd899e688cf932c0c236c2d6b24`
- Latest Switch loader side-effect audit Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_03-58-14.log`
- Latest restored default ExeFS Build ID:
  `49a1b018938f7a716d73a11ed9ff2256a19d4af6`
- Latest restored default ExeFS Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-16_04-02-06.log`
- Latest default expected behavior:
  `main entered` -> `Switch audit package startup` -> `Early preflight missing game executable: sdmc:/switch/LibertyRecomp/game/default.xex`

## Verification Commands

Configure default ExeFS:

```powershell
$build='C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug'
Remove-Item Env:VCPKG_ROOT -ErrorAction SilentlyContinue
$env:DEVKITPRO='C:/devkitPro'
$env:DEVKITA64='C:/devkitPro/devkitA64'
$cmake='C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake -S 'C:\Users\Jellybone\Documents\GitHub\LibertyRecomp' -B $build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDEVKITPRO=C:/devkitPro -DDEVKITA64=C:/devkitPro/devkitA64 -DCMAKE_TOOLCHAIN_FILE='C:\Users\Jellybone\Documents\GitHub\LibertyRecomp\toolchains\switch-libnx.cmake' -DCMAKE_MAKE_PROGRAM='C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\bin\ninja.exe' -DLIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONFIG_LOAD=OFF -DLIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_INSTALL_CHECK=OFF -DLIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONTENT_LAYOUT_CHECK=OFF -DLIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_VFS_PREFLIGHT=OFF -DLIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT=OFF -DLIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT=OFF
```

Build default ExeFS:

```powershell
$env:DEVKITPRO='C:/devkitPro'
$env:DEVKITA64='C:/devkitPro/devkitA64'
& 'C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\bin\ninja.exe' -C 'C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug' -j 2 LibertyRecompExeFs
```

Read Build ID and TEXTREL status:

```powershell
& 'C:\devkitPro\devkitA64\bin\aarch64-none-elf-readelf.exe' -n 'C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecomp' | Select-String -Pattern 'Build ID'
& 'C:\devkitPro\devkitA64\bin\aarch64-none-elf-readelf.exe' -dW 'C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug\LibertyRecomp\LibertyRecomp' | Select-String -Pattern 'TEXTREL|FLAGS'
```

Ryujinx paths:

```text
Ryujinx executable: D:\Games\Ryujinx\Ryujinx\Ryujinx.exe
Ryujinx logs: D:\Games\Ryujinx\Ryujinx\portable\Logs
Ryujinx SD root: D:\Games\Ryujinx\Ryujinx\portable\sdcard
Real GTA IV source: D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)
```

## Next Stage Entry Conditions

Enter the next stage only after this stage has:

- a pushed commit,
- updated `CONTINUATION_GUIDE.md`,
- fresh build/readelf/Ryujinx verification evidence,
- no temporary real-content links left in Ryujinx SD,
- Ryujinx PTC restored to its original value,
- and a documented pivot decision: either return mainline work to Windows runtime code, or name one exact Switch pre-guest blocker to continue.

## Next Boundary Decision

The pivot decision is now made: pause Switch feature work, keep `LibertyRecompExeFs` as the regression package, and move the mainline to Windows runtime / unfinished upstream code. Switch should only resume for regression failures, packaging/content-layout fixes, or a deliberately scoped pre-guest blocker. Do not claim Switch playability.
