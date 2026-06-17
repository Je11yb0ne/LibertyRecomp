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

2026-06-17 Windows wrapper continuation 45:

- Confirmed `sub_8221F8A8` was another wrapper-recursion frame. The project-side wrapper called `sub_8221F8A8(ctx, base)` while ReXGlue generated `PPC_FUNC_IMPL(__imp__sub_8221F8A8)` exists in `glue/rexglue-sdk-main/gta4-recomp/generated/gta4_recomp.6.cpp`.
- Added a narrow static regression check before editing; it failed while the wrapper called the public alias and passed after the fix.
- Minimal fix: preserved the public `extern "C" void sub_8221F8A8(...)` declaration, added `extern "C" void __imp__sub_8221F8A8(...)`, and changed only the logging wrapper body to call `__imp__sub_8221F8A8(ctx, base)`.
- Fresh Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Fresh Windows build result: success. Warnings remain the existing `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the `ctx.lr` printf format warning.
- Fresh Windows smoke logs:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-7052169a-1343-42a1-a4f3-6ad39fe5d140.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-9dbff4be-155b-41fe-8093-0c1dbb70a24a.log`
- Fresh smoke result: process exit code was `0x00000000`, but the VEH log still records `0xC00000FD` stack overflow. The run preserved the current VFS progress through `platform:/textures/buttons_360` before the stack overflow trace.
- Latest repeated frame is RVA `0x80F05`; the map places `sub_82273988` at `0x140080400`, and objdump shows `0x140080F00` is a call back to `0x140080400`, with return address `0x140080F05`. This confirms the next stack frame is `sub_82273988` wrapper recursion.
- Smoke execution note: verification used the same 15-second bounded `Start-Process` smoke and cleaned the temporary `portable.txt`, `game/default.xex`, and empty `game` directory from the build tree.
- Push note: local commits remain unpushed because GitHub credential-manager requires an interactive prompt. Continue committing locally until credentials are restored, then push the accumulated branch.
- Next Windows boundary: inspect `sub_82273988`; confirm generated `__imp__sub_82273988` and the exact wrapper call pattern before editing.

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

1. Inspect the Windows stack overflow now landing in `sub_82273988` wrapper recursion.
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

## 2026-06-17 Windows Continuation 46: Startup Wrapper Batch 1

Current mainline goal:

- Continue Windows runtime bring-up with ReXGlue generated implementations as primary and XenonRecomp as auxiliary reference.
- Clear project-side wrapper self-recursion in the startup path, then continue into real VFS/resource loading, thread synchronization, MMIO/VBlank, and GPU initialization blockers.
- Do not treat individual wrapper fixes as a final phase boundary; batch small verified wrapper fixes, record evidence, commit, push, and continue.

Completed in this batch:

- `sub_82273988`: resource-array initialization wrapper now calls `__imp__sub_82273988`.
- `sub_82124080`: profile/save init wrapper now calls `__imp__sub_82124080`.
- `sub_82124540`: stream/config parser wrapper now calls `__imp__sub_82124540` in both invalid-buffer and normal paths.
- `sub_82192840`: file-open wrapper now calls `__imp__sub_82192840` before handle validation.
- `sub_82192980`: file stream read wrapper now calls `__imp__sub_82192980` before buffer logging.

Fresh verification:

- RED/GREEN static checks were run for each repaired wrapper; the final combined GREEN check reported:
  `GREEN_PASS_current_five_wrappers_call_imp`.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-e8aa57e6-2f74-4458-b425-1ec3e9b0aa3d.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-d0c55056-6d1d-4244-ba12-9058e0c758a0.log`
- Smoke result:
  exited `0x00000000` after the VEH handler logged stack overflow; startup reached MMIO bridge writes, VBlank ticks, guest thread launches, VFS resolves for `platform:/textures/fonts` and `platform:/textures/buttons_360`, then `common:/DATA/LOADINGSCREENS_360.DAT`.
- The latest repeated VEH frame moved to:
  `rva=0x83772`.
- Map result:
  `rva=0x83772` maps to `sub_821244B8 + 0x1F2` in `imports.cpp.obj`.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Fix `sub_821244B8` if RED check confirms direct wrapper recursion.
   Completion standard: ReXGlue `__imp__sub_821244B8` declaration exists, wrapper no longer calls itself, build succeeds, smoke moves past `rva=0x83772`.
2. Continue along the current file stream/profile chain instead of jumping to unrelated static-scan results.
   Completion standard: each next repeated RVA is mapped through `LibertyRecomp.map` before editing.
3. Keep the static self-recursion scan as a backlog, not a bulk rewrite.
   Completion standard: only fix a scanned wrapper when it is on the current smoke call path or when a narrow batch is explicitly justified.
4. Watch for the first non-wrapper blocker.
   Completion standard: if the repeated frame maps outside `imports.cpp.obj` or no longer contains a direct same-symbol call, switch to root-cause tracing instead of wrapper replacement.
5. Commit and push each verified batch.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_821244B8`, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-17 Windows Continuation 47: Startup Wrapper Batch 2

Current mainline goal:

- Continue Windows runtime bring-up without stopping at individual wrapper fixes.
- Keep using ReXGlue generated `__imp__*` implementations as the primary target and XenonRecomp only as auxiliary reference.
- Maintain Switch as a frozen pre-guest regression baseline unless a Switch-specific regression check is explicitly needed.

Completed in this batch:

- `sub_821244B8`: startup file/profile wrapper now calls `__imp__sub_821244B8`.
- `sub_8221D880`: config/resource wrapper now calls `__imp__sub_8221D880`.
- `sub_8219FD88`: file/resource chain wrapper now calls `__imp__sub_8219FD88`.
- `sub_8230D760`: loop helper wrapper now calls `__imp__sub_8230D760`.
- `sub_8230D160`: loop helper wrapper now calls `__imp__sub_8230D160`.

Fresh verification:

- Combined static GREEN check reported:
  `GREEN_PASS_current_five_wrappers_call_imp`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-c12a4018-d179-4429-a0fa-e8aaee0a1ccc.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-e440b9cc-55ce-4979-b8f8-35fe63fd0fb6.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. It moved past the old repeated frames `0x83772`, `0x7BDB4`, `0xA0A92`, `0x9F2AB`, and `0x9F786`, reached MMIO bridge writes, VBlank ticks, guest thread launches, VFS resolves for `platform:/textures/fonts` and `platform:/textures/buttons_360`, and content probes for `common:/DATA/LOADINGSCREENS_360.DAT`, `platform:/engineSettings.xml`, and `platform:/config/curves.dat`.
- The latest repeated VEH frame moved to:
  `rva=0x9FC54`.
- Map result:
  `rva=0x9FC54` maps to `sub_8219F9A0 + 0x1F4` in `imports.cpp.obj`.
- Current root-cause check:
  `sub_8219F9A0` is another direct wrapper self-recursion and should be the next fix target.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Fix `sub_8219F9A0` if RED check confirms direct wrapper recursion.
   Completion standard: ReXGlue `__imp__sub_8219F9A0` declaration exists, wrapper no longer calls itself, build succeeds, smoke moves past `rva=0x9FC54`.
2. Check adjacent `sub_8219F948` only if smoke maps to it or if a narrow same-chain batch is justified by the existing trace.
   Completion standard: no bulk static rewrite; each edited wrapper has a mapped or directly adjacent startup-chain reason.
3. Continue mapping repeated VEH frames through `LibertyRecomp.map`.
   Completion standard: every next edit is backed by a smoke frame or a same-chain direct recursion check.
4. Watch for the first non-wrapper blocker.
   Completion standard: if the repeated frame leaves `imports.cpp.obj` or no same-symbol wrapper recursion exists, stop wrapper replacement and switch to root-cause tracing.
5. Commit and push the next verified batch.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_8219F9A0`, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-17 Windows Continuation 48: Startup Wrapper Batch 3

Current mainline goal:

- Continue Windows runtime bring-up through the current startup-chain wrapper layer.
- Use ReXGlue generated `__imp__*` implementations as the primary target; do not bulk-rewrite unrelated instrumentation wrappers.
- Keep Switch frozen as the pre-guest regression baseline unless a Switch-specific regression is explicitly scoped.

Completed in this batch:

- `sub_8219F9A0`, `sub_8219F948`, and `sub_824C1668`: adjacent `sub_8219FD88` internal wrappers now call generated `__imp__*` implementations.
- `sub_822F8980`: storage/file init wrapper now preserves `g_inStorageInit` instrumentation while calling `__imp__sub_822F8980`.
- `sub_82270170`, `sub_822FD328`, `sub_822EFF40`, `sub_82120C48`, and `sub_82221410`: early subsystem wrappers now call generated `__imp__*` implementations.
- `sub_8214B508`, `sub_8214B570`, `sub_8214B640`, `sub_8214B6A8`, `sub_825B8380`, and `sub_823A70A8`: `sub_821E9658` internal wrappers now call generated `__imp__*` implementations.
- `sub_822E2510` and `sub_8214C488`: `sub_82126940` internal wrappers now call generated `__imp__*` implementations.
- `sub_821E9658`, `sub_821FD460`, `sub_82126940`, `sub_822B6C58`, `sub_82308598`, and `sub_82209280`: `sub_82120C48` internal wrappers now call generated `__imp__*` implementations.

Fresh verification:

- Combined static GREEN check reported:
  `GREEN_PASS_CURRENT_23_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-ac825a78-466a-4819-99e2-2b05b63fabe5.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-219d0690-c2fb-47a4-af56-f3d4d02f27e0.log`
- Smoke result:
  exited `0x00000000` after the VEH handler logged stack overflow. It moved past `sub_8219F9A0`, `sub_822F8980`, `sub_82270170`, and `sub_821E9658`; the `sub_821E9658` and `sub_82126940` internal wrappers now log ENTER/EXIT instead of recursing. Startup still reaches MMIO bridge writes, VBlank ticks, guest thread launches, VFS probes for platform texture paths, and content probes including `common:/DATA/LOADINGSCREENS_360.DAT`, `platform:/engineSettings.xml`, `platform:/config/curves.dat`, and `platform:/`.
- The latest repeated VEH frame moved to:
  `rva=0x86BBD`.
- Map result:
  `rva=0x86BBD` maps to `sub_8260E310 + 0x20D` in `imports.cpp.obj`.
- Current root-cause check:
  `sub_8260E310` is the next UI-internal startup wrapper to inspect before editing.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/*.patch`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_8260E310` and the adjacent `sub_82221410` UI-internal wrapper group.
   Completion standard: RED check confirms direct wrapper recursion or identifies a different root cause before editing.
2. If the UI-internal group is direct recursion, patch only that mapped same-chain group to call generated `__imp__*`.
   Completion standard: static GREEN check proves no same-symbol calls remain in edited wrappers.
3. Rebuild and run bounded smoke.
   Completion standard: Windows build succeeds, smoke moves past `rva=0x86BBD`, and the next repeated frame is mapped.
4. Stop wrapper replacement when the repeated frame leaves direct instrumentation recursion.
   Completion standard: switch to root-cause tracing for the first non-wrapper blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe` for push.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_8260E310`, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-17 Windows Continuation 49: Startup Wrapper Batch 4

Current mainline goal:

- Continue Windows runtime bring-up through the UI/GPU trace wrapper layer.
- Preserve existing hand-written bypass stubs for known Xbox GPU/device/scheduler blockers.
- Keep Switch frozen as the pre-guest regression baseline unless a Switch-specific regression is explicitly scoped.

Completed in this batch:

- `sub_8260E310`, `sub_822B3C58`, `sub_822B4D68`, `sub_824A0898`, `sub_8260CF30`, `sub_8260BE08`, `sub_824B65B0`, `sub_822E3EC8`, and `sub_8222F7E8`: `sub_82221410` UI-internal wrappers now call generated `__imp__*` implementations.
- `sub_827E0740`, `sub_8285F750`, and `sub_82860928`: `sub_824A0898` nested wrappers now call generated `__imp__*` implementations.
- `sub_821EC018`, `sub_827DFC60`, and `sub_8285DC80`: `sub_822B4D68` nested wrappers now call generated `__imp__*` implementations.
- `sub_828787C0`, `sub_8286DA20`, `sub_8286D668`, `sub_827E93F8`, `sub_8286BAE0`, and `sub_829E5C38`: GPU resource trace wrappers now call generated `__imp__*` implementations.
- Preserved intentional stubs/bypasses:
  `sub_8249D6F0`, `sub_822E49A0`, `sub_8285AF80`, and `sub_82850028`.

Fresh verification:

- UI static GREEN check reported:
  `GREEN_PASS_UI_INTERNAL_WRAPPERS_CALL_IMP_STUBS_PRESERVED`.
- UI/GPU static GREEN check reported:
  `GREEN_PASS_UI_AND_GPU_TRACE_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-2df532c2-e47b-4c71-93af-cd6ffdd32532.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-39614655-8681-4137-b597-93a782573d5b.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. It moved past `sub_8260E310`, the UI-internal wrapper group, and the GPU trace wrapper group. Startup now reaches additional embedded/platform DCL content probes such as `embedded:/dcl/dcl`, `embedded:/dcl`, `platform:/dcl/dcl/dcl`, and `platform:/dcl/dcl`, while preserving the earlier MMIO/VBlank/guest-thread/VFS progress.
- The latest repeated VEH frame moved to:
  `rva=0x8E054`.
- Map result:
  `rva=0x8E054` maps to `sub_8226CB50 + 0x1F4` in `imports.cpp.obj`.
- Current root-cause check:
  `sub_8226CB50` is the next Camera system startup wrapper to inspect before editing.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/*.patch`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_8226CB50` and the adjacent camera/subsystem wrapper group.
   Completion standard: RED check confirms direct wrapper recursion or identifies a different root cause before editing.
2. Patch only the mapped same-chain wrapper group if it is direct instrumentation recursion.
   Completion standard: static GREEN check proves no same-symbol calls remain in edited wrappers.
3. Rebuild and run bounded smoke.
   Completion standard: Windows build succeeds, smoke moves past `rva=0x8E054`, and the next repeated frame is mapped.
4. Stop wrapper replacement when the repeated frame leaves direct instrumentation recursion.
   Completion standard: switch to root-cause tracing for the first non-wrapper blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe` for push.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_8226CB50`, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-18 Windows Continuation 50: Camera/HUD/Menu Wrapper Batch

Current mainline goal:

- Continue Windows runtime bring-up through the startup subsystem wrapper layer.
- Keep ReXGlue generated `__imp__*` implementations as the primary implementation target for project-side wrappers.
- Do not treat this wrapper batch as a stopping point; continue into the next real VFS/resource, thread, MMIO/VBlank, GPU, or function-pointer blocker after recording and pushing.

Completed in this batch:

- `sub_8226CB50`: camera system wrapper now calls `__imp__sub_8226CB50`.
- `sub_821A8278`: HUD component wrapper now calls `__imp__sub_821A8278` while preserving the surrounding HUD diagnostic logging.
- `sub_821BC9E0`: menu system wrapper now calls `__imp__sub_821BC9E0`.
- Preserved `sub_821A8868` as an expanded hand-written HUD initialization reimplementation; it was not converted to a generated call-through wrapper.

Fresh verification:

- Static GREEN check reported:
  `GREEN_PASS_CAMERA_HUD_MENU_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-4163f911-1e9d-401a-bb63-13a1b98223cd.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-69c6e8fa-8b2e-4abc-9cb7-a3370aeb1009.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. It moved past the camera wrapper, HUD init, HUD component, and menu wrapper recursion. Startup still reaches graphics backend attempts, MMIO bridge writes, VBlank ticks, guest thread launches, VFS probes for `platform:/textures/fonts` and `platform:/textures/buttons_360`, and content probes for `common:/DATA/LOADINGSCREENS_360.DAT`, `platform:/engineSettings.xml`, `platform:/config/curves.dat`, `embedded:/dcl/*`, and `platform:/dcl/*`.
- The latest repeated VEH frame moved to:
  `rva=0x79EC6`.
- Map result:
  `rva=0x79EC6` maps to `sub_827DA8E0 + 0x226` in `imports.cpp.obj`.
- Current stderr also logs:
  `[MISSING-FUNC] indirect call to 01000000 (in_range=0)`.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/*.patch`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_827DA8E0` before editing.
   Completion standard: identify whether `sub_827DA8E0 + 0x226` is direct wrapper recursion, a bad indirect-call dispatch path, or a deeper function-pointer/VFS side effect.
2. Trace the `01000000` indirect call source.
   Completion standard: determine whether `01000000` is a corrupted guest function pointer, a placeholder/null-adjacent host pointer, or an expected guest address that is missing from the generated dispatch table.
3. Make the smallest root-cause fix.
   Completion standard: if it is wrapper recursion, redirect only the wrapper to generated `__imp__*`; if not, add diagnostic evidence at the failing boundary before changing runtime behavior.
4. Rebuild and run bounded smoke.
   Completion standard: Windows build succeeds, smoke moves past `rva=0x79EC6` or produces a clearly different mapped blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe`.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_827DA8E0` and the `01000000` indirect call, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-18 Windows Continuation 51: Allocator/SubSystem Wrapper Batch

Current mainline goal:

- Continue Windows runtime bring-up past the remaining project-side startup wrappers and into repeatable resource/VFS loading.
- Keep ReXGlue generated `__imp__*` implementations as the implementation target for logging wrappers.
- Record progress in batches but continue immediately after commit; this is not a playability milestone.

Completed in this batch:

- `sub_827DA8E0`: large-allocation vtable wrapper now calls `__imp__sub_827DA8E0`.
- `sub_827D9C50`: large-allocation helper wrapper now calls `__imp__sub_827D9C50`.
- `sub_822DB4B0`, `sub_821B7218`, and `sub_822498F8`: Cutscene, Mission, and Checkpoint subsystem wrappers now call generated `__imp__*` implementations.
- `sub_8225DC40`, `sub_821E24E0`, `sub_821DFD18`, `sub_8220E108`, `sub_821D8358`, `sub_821EA0B8`, and `sub_82200EB8`: Weather, Population, Traffic, Wanted, Map/GPS, Blip, and Stats subsystem wrappers now call generated `__imp__*` implementations.

Fresh verification:

- Static GREEN check reported:
  `GREEN_PASS_ALLOC_AND_SUBSYSTEM_BATCH_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp docs/switch-audit/CONTINUATION_GUIDE.md` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-4f25601b-3adb-4e29-b856-b91dbd2ea744.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-f18d0b8a-c29b-4a29-bb45-264f362e6943.log`
- Smoke result:
  exited `0x00000000` after the VEH handler logged stack overflow. It moved past allocator wrappers and the `[63-SUBSYS]` Weather/Stats startup wrapper chain. It now reaches a deeper resource-loading surface with `platform:/materials`, `platform:/fragmentxml`, `platform:/xrt`, `/taskParams.txt`, many `platform:/data/decision/*.ped`, and `platform:/data/decision/Combat/*.cmb` probes.
- Current VFS note:
  empty `GTA::FileResolve` requests currently resolve to the build `game\` directory and report success; this is now visible evidence for a future VFS/resource-boundary audit.
- Current stderr note:
  `[MISSING-FUNC] indirect call to 01000000 (in_range=0)` still appears, and later smoke also logs missing indirect calls to `00000000`. Treat these as follow-up dispatch/function-pointer blockers after the immediate repeated-frame wrapper is handled.
- The latest repeated VEH frame moved to:
  `rva=0x70BCA`.
- Map result:
  `rva=0x70BCA` maps to `sub_827D85E0 + 0x6A` in `imports.cpp.obj`.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/*.patch`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_827D85E0` before editing.
   Completion standard: confirm whether it is direct wrapper recursion with a generated `__imp__sub_827D85E0`, or a different allocator/dispatch issue.
2. Continue tracking missing indirect calls.
   Completion standard: after `sub_827D85E0` is cleared, determine whether `01000000` and `00000000` are still present and whether they are causal or background diagnostics.
3. Investigate the empty-path VFS success path once wrapper recursion is no longer the top repeated frame.
   Completion standard: empty path resolution behavior is either intentionally tolerated for startup or changed with evidence from the caller path.
4. Rebuild and run bounded smoke after each minimal fix.
   Completion standard: Windows build succeeds and smoke either moves past `rva=0x70BCA` or produces a clearly different mapped blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe`.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_827D85E0`, still in Windows startup bring-up.
- Switch remains frozen as a regression baseline; do not build `LibertyRecompExeFs` or `LibertyRecompNro` unless Windows changes require Switch regression testing or a Switch-specific task is explicitly scoped.

## 2026-06-18 Windows Continuation 52: Render/ORCH Wrapper Batch

Current mainline goal:

- Continue Windows runtime bring-up in larger verified batches instead of stopping after each single wrapper.
- Keep using ReXGlue/reference-generated `__imp__*` implementations where wrappers were only logging shells.
- Keep Switch frozen as the regression baseline; this batch is Windows-only and not a playability milestone.

Completed in this batch:

- `sub_827D85E0` and `sub_827D8620`: allocator/resource helper wrappers now call generated `__imp__*` implementations.
- `sub_8285E6E8`, `sub_8286BBE8`, `sub_8286A970`, `sub_8285E2C0`, `sub_8285E6C0`, `sub_829D33B8`, and `sub_82871A18`: render/texture setup wrapper cluster now calls generated `__imp__*` implementations.
- `sub_8285ACE8`, `sub_829CA360`, `sub_829CA240`, `sub_829D14E0`, `sub_82852610`, and `sub_829D5920`: ORCH/render loop wrappers now call generated `__imp__*` implementations.
- `sub_82853CB0` and `sub_829D87E8` were intentionally left unchanged because they are deliberate host-side bypasses/hook shims in the existing runtime notes.

Fresh verification:

- Static GREEN checks reported:
  `GREEN_PASS_sub_827D85E0_sub_827D8620_decl_and_call_imp`,
  `GREEN_PASS_ALLOC_RENDER_CLUSTER_WRAPPERS_CALL_IMP`,
  and `GREEN_PASS_ORCH_RENDER_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-4c375118-ed4c-4d09-81e2-e8698c583d1a.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-c54b663c-490f-460e-a71e-13d1001d4cff.log`
- Smoke result:
  exited `0x00000000` after the VEH handler logged stack overflow. It moved past the prior `sub_827D85E0`, `sub_827D8620`, `sub_8285E6E8`, `sub_8285ACE8`, and ORCH/render wrapper recursion blockers.
- Current runtime surface:
  startup still reaches graphics backend attempts, VBlank ticks, guest thread launches, `platform:/textures/*`, `platform:/materials`, `platform:/fragmentxml`, `platform:/xrt`, `/taskParams.txt`, and `platform:/data/decision/*` probes.
- Current stderr note:
  `[MISSING-FUNC] indirect call to 829F6F60` through nearby in-range guest addresses now appears before xstart, and the older `01000000`/`00000000` missing indirect calls remain later. Treat these as dispatch/function-pointer follow-ups after the immediate repeated-frame blocker is inspected.
- The latest repeated VEH frame moved to:
  `rva=0x89DC3`.
- Map result:
  `rva=0x89DC3` maps to `sub_828536B0 + 0x203` in `imports.cpp.obj`.
- Supporting stack frames:
  `rva=0x1B5B7`, `rva=0x125BF`, and `rva=0x1210A` map into `fmt` formatting functions in `main.cpp.obj`, likely reached from logging inside or below `sub_828536B0`.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_828536B0` before editing.
   Completion standard: determine whether `sub_828536B0 + 0x203` is wrapper recursion, logging/fmt recursion, or a deeper render/resource loop.
2. Compare `sub_828536B0` against `imports.cpp.old_runtime_backup` and `work/refs/GTA4Recomp`.
   Completion standard: identify whether the correct behavior is generated `__imp__*`, host bypass, or additional diagnostics.
3. Continue tracking in-range missing indirect calls around `829F6F60` through `829F9DC8`.
   Completion standard: determine whether these are expected guest callback table entries missing from generated dispatch, or background diagnostics unrelated to the current stack overflow.
4. Rebuild and run bounded smoke after the next minimal fix.
   Completion standard: Windows build succeeds and smoke either moves past `rva=0x89DC3` or produces a clearly different mapped blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_828536B0`, still in Windows startup/render-resource bring-up.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 53: Deep Init and 63-SUBSYS Wrapper Batch

Current mainline goal:

- Continue Windows runtime bring-up through larger confirmed wrapper clusters and stop treating a single wrapper as a stage.
- Keep using backup/reference ReXGlue `__imp__*` implementations for logging-only wrappers.
- Do not claim playability; this remains startup/resource/runtime bring-up.

Completed in this batch:

- `sub_82126498`, `sub_828536B0`, `sub_8260E3B8`, `sub_8260E3D8`, `sub_8286A748`, and `sub_8286A890`: `sub_8249D6F0` deep-instrumentation wrappers now call generated `__imp__*` implementations.
- `sub_8212FB78`, `sub_8219ADF0`, `sub_8212F578`, `sub_8212EDC8`, `sub_82138710`, `sub_821B2ED8`, `sub_822467B8`, `sub_82208460`, `sub_821B9DA8`, `sub_82258100`, `sub_821A03A0`, `sub_8232A2C0`, and `sub_82125478`: 63-SUBSYS/friend/online/camera/TV/phone/dating/final setup wrappers now call generated `__imp__*` implementations.

Fresh verification:

- Static GREEN checks reported:
  `GREEN_PASS_8249D6F0_INTERNAL_WRAPPERS_CALL_IMP`
  and `GREEN_PASS_63_SUBSYS_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-8062d18c-129f-454b-b518-3e2ecd87eddf.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-6aeac361-7ae9-4ba4-bf01-9a094630dc81.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. It moved past the previous `sub_828536B0` and `sub_8212FB78` repeated-frame blockers.
- New runtime surface:
  smoke now reaches deeper DCL scans, `platform:/introSpline.csv`, `platform:/introLoc.csv`, `platform:/stockshake.txt`, `platform:/trainCamNodes.txt`, Stats system exit, and then a semaphore/sync-table hot loop.
- Current stdout note:
  `sub_827DAD60` logs repeated `[SEM_SIGNAL] ... SYNC_TABLE handle=0xA84E73F0 release=2198819880`, reaching over 12,000 iterations before the 15-second kill.
- Current stderr note:
  the in-range missing indirect calls around `829F6F60` still appear before xstart, and repeated `00000000` missing indirect calls remain later.
- Latest repeated VEH frames:
  `rva=0x79D81` and `rva=0x79D4B`.
- Map result:
  `rva=0x79D81` maps to `sub_827DAD60 + 0x91`, and `rva=0x79D4B` maps to `sub_827DAD60 + 0x5B` in `imports.cpp.obj`.
- Supporting stack frames:
  `rva=0x737A` maps to `printf` in `main.cpp.obj`, and `rva=0x3B43DD6` maps to UCRT `__stdio_common_vfprintf`, consistent with logging pressure inside the semaphore path.
- Temporary smoke files `portable.txt`, `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect `sub_827DAD60` before editing.
   Completion standard: identify whether the hot loop is caused by logging volume, missing semaphore state transition, wrong handle lookup, or a guest sync wait/signal mismatch.
2. Compare `sub_827DAD60` against backup/reference runtime behavior.
   Completion standard: determine whether this path should throttle logging, update sync-table state differently, or route to a generated implementation.
3. Correlate `sub_827DAD60` with the missing indirect-call stream.
   Completion standard: determine whether `00000000` indirect calls are causing the sync storm or are separate guest-thread diagnostics.
4. Rebuild and run bounded smoke after the next minimal fix.
   Completion standard: Windows build succeeds and smoke either moves past the `sub_827DAD60` hot loop or produces a clearly different bounded blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `sub_827DAD60` semaphore/sync-table hot loop, still in Windows startup/resource bring-up.
- Switch remains frozen unless a scoped Switch regression check is explicitly needed.

## 2026-06-18 Windows Continuation 54: Semaphore and Finalization Wrapper Batch

Current mainline goal:

- Continue Windows runtime bring-up beyond the instrumentation wrapper layer and into repeatable VFS/resource/function-dispatch blockers.
- Keep ReXGlue generated `__imp__*` implementations as the target for project-side logging wrappers.
- Record and commit this verified boundary, then continue immediately; this is not a playability milestone.

Completed in this batch:

- `sub_827DAD60`: semaphore/sync-table signal wrapper now calls generated `__imp__sub_827DAD60` after preserving the existing `SyncTable_Signal()` diagnostic semantics.
- `sub_8227AC28`, `sub_82272290`, `sub_82212450`, `sub_822C5768`, and `sub_822D4C68`: final 63-SUBSYS finalization wrappers now call generated `__imp__*` implementations instead of recursively calling their own public aliases.
- The finalization cluster was confirmed against `imports.cpp.old_runtime_backup` and generated ReXGlue sources before editing.

Fresh verification:

- Static GREEN check reported:
  `GREEN_PASS_SEMAPHORE_FINALIZATION_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known 5 `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-short-e4817db6-f2dc-4715-bbbc-dcc03feb417f.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-short-42ca5d4f-95b9-45fc-8b07-f1e356943779.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. It moved past the previous `sub_827DAD60` semaphore hot loop and the `sub_8227AC28` finalization recursion frame. No VEH stack overflow dump appeared in this smoke log.
- New runtime surface:
  startup reaches deeper FX/resource probing, repeated `GuestThread` launch/return traces at `0x829B08E0`, `platform:/textures/hud`, `platform:/textures/skydome`, `platform:/textures/fx_Rain`, `platform:/data/effects/*`, and final setup exit.
- Current stderr note:
  in-range missing indirect calls remain at `829F6F60`, `829F6F80`, `829F6FA0`, `829F6FC0`, `829F7020`, `829F70E0`, `829F7100`, `829F71E8`, and `829F9DC8`; one `01000000` call remains; the dominant current repeated diagnostic is `107` occurrences of `[MISSING-FUNC] indirect call to 00000000 (in_range=0)`.
- Current VFS note:
  empty `GTA::FileResolve` requests still resolve as success to the build `game\` directory. Treat this as the next VFS/resource-boundary audit candidate, but do not change it without tracing the caller.
- Temporary smoke files `portable.txt`, copied `game/default.xex`, and empty `game` directory were removed from the Windows build output after testing. A first attempted hardlink smoke was invalid because Windows cannot hardlink across drives; the verified smoke used `Copy-Item`.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Trace the `00000000` indirect-call storm.
   Completion standard: identify the guest call site, register/source value, and whether this is an uninitialized callback slot, bad VFS/resource side effect, or expected nullable callback path.
2. Inspect in-range missing indirect calls around `829F6F60..829F9DC8`.
   Completion standard: determine whether they are missing generated function-table entries, data/callback table addresses, or benign one-time diagnostics.
3. Audit empty-path VFS success.
   Completion standard: trace at least one empty `GTA::FileResolve` caller and decide whether empty paths should fail, no-op, or keep resolving to `game\` during startup.
4. Add the smallest diagnostic or runtime fix for the first proven root cause.
   Completion standard: no behavior change unless the caller/source is identified; if it is another wrapper recursion, redirect only the wrapper to `__imp__*`.
5. Rebuild and run bounded smoke after the next minimal fix.
   Completion standard: Windows build succeeds and smoke either reduces the `00000000` indirect-call storm or produces a clearly different mapped blocker.
6. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at the `00000000` missing indirect-call storm and empty-path VFS behavior, still in Windows startup/resource bring-up.
- Switch remains frozen unless a scoped Switch regression check is explicitly needed.
