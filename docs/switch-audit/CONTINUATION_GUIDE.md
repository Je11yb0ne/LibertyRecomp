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

GTA IV-scale codegen memory-pressure note: `docs/dev/REVERSE_ENGINEERING_TOOLS.md` records OZORDI/XenonRecomp commit `207253d67cdef67235805d595999fa0a2e4fcbd9`, which fixed unbounded memory growth caused by `static` local scratch containers inside `Recompile()`. If future XenonRecomp/ReXGlue codegen runs OOM or grows for hours, audit per-function scratch `std::string`, `std::vector`, and `std::unordered_set` use before changing generated output.

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

## 2026-06-18 Windows Continuation 55: Indirect-Call Source Diagnostics

Current mainline goal:

- Continue Windows runtime bring-up past wrapper-recursion cleanup into function-dispatch, VFS/resource, and renderer-preflight blockers.
- Preserve the current ReXGlue-generated dispatch behavior while collecting enough source-register evidence to stop guessing at missing indirect calls.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Added bounded diagnostics to `PPC_CALL_INDIRECT_FUNC` in `glue/rexglue-sdk-main/include/rex/ppc/context.h`.
- Missing indirect calls now log the first 256 call sites with target address, `in_range`, `lr`, `ctr`, `r1`, and `r12`.
- The diagnostic is intentionally behavior-preserving: it does not install fallback handlers, skip calls, or alter guest register state.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- glue/rexglue-sdk-main/include/rex/ppc/context.h docs/switch-audit/CONTINUATION_GUIDE.md` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-indirect-fresh-dd53455e-5a33-4cfe-859b-9bca0bf73138.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-indirect-fresh-7dfef8f1-9d87-438a-8413-1d0d2df2e151.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. The diagnostic shows the first in-range missing targets come from the static constructor table executor at `lr=0x829A7E80`, and the later `00000000` calls come from distinct resource/render/finalization paths rather than one anonymous repeated site.

Key evidence:

- In-range missing constructor targets:
  `0x829F6F60`, `0x829F6F80`, `0x829F6FA0`, `0x829F6FC0`, `0x829F7020`, `0x829F70E0`, `0x829F7100`, `0x829F71E8`, and `0x829F9DC8`, all with `lr=0x829A7E80`.
- Out-of-range non-null target:
  `0x01000000` with `lr=0x82121160`.
- Dominant null targets include call sites around:
  `0x82753D70`, `0x824315D4`, `0x821C5020`, `0x822F0684`, `0x822F97A8`, `0x82200F74`, `0x827513F4`, `0x8275C440`, `0x8251D638`, and `0x823190D4`.
- The smoke still reaches repeated resource probes such as `platform:/textures/fx_Rain`, embedded DCL lookups, and final setup traces; this remains startup/resource/runtime bring-up, not playability.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `glue/rexglue-sdk-main/include/rex/ppc/context.h`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`,
  and, for the next fix stage only after root-cause confirmation, `LibertyRecomp/kernel/imports.cpp`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Inspect the constructor-table missing targets around `0x829F6F60..0x829F9DC8`.
   Completion standard: determine whether these are valid function starts omitted from `PPCFuncMappings`, data/callback table entries, or thunks that ReXGlue intentionally skipped.
2. Trace the first non-null out-of-range target `0x01000000`.
   Completion standard: identify the source value at `lr=0x82121160` and whether it is a malformed function pointer, endian/flagged pointer, or expected nullable dispatch path.
3. Trace the highest-volume `00000000` call-site cluster.
   Completion standard: choose one repeated LR cluster, map it to a generated function, and identify the guest field/table slot feeding CTR.
4. Add the smallest next diagnostic or runtime fix for the first proven root cause.
   Completion standard: no behavior change unless the caller/source is identified; if the fix is generated-dispatch coverage, first prove it with a temporary ReXGlue codegen/diff or IDA/source evidence.
5. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either removes/reclassifies the target missing-call cluster or produces a clearly different blocker.
6. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at constructor-table target coverage and the `01000000` / `00000000` indirect-call source tracing.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 56: Forced Constructor Target Coverage

Current mainline goal:

- Continue Windows runtime bring-up by turning missing indirect-call diagnostics into narrow runtime coverage fixes.
- Avoid a tracked generated-code refresh until the ReXGlue 0.8.1 generated-output migration is deliberately scoped.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Proved with a temporary ReXGlue codegen run that constructor-table targets `0x829F6F60`, `0x829F6F80`, `0x829F6FA0`, `0x829F6FC0`, `0x829F7020`, `0x829F70E0`, `0x829F7100`, `0x829F71E8`, and `0x829F9DC8` are valid recompiled function entries, not data table artifacts.
- Temporary codegen output:
  `C:\Users\JELLYB~1\AppData\Local\Temp\rexglue-forced-ctor-cc9fc38c-83e8-4050-9c8f-101629f95d13\generated`
- Added narrow hand-written equivalents for those nine ctor thunk functions in `LibertyRecomp/kernel/imports.cpp`, matching the temporary ReXGlue output.
- Registered the nine thunk functions dynamically before `sub_829A7DC8` enters the table2 constructor loop.
- This avoids editing the tracked generated directory for this boundary while preserving the current legacy runtime shape.

Fresh verification:

- Static check confirmed all nine `PPC_FUNC(sub_829F...)` definitions and the `LibertyRegisterForcedCtorTargets()` call exist.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known five `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-ctorforced-6ff159f3-8013-4dfa-a381-db561ed886f4.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-ctorforced-9af32841-f375-443e-9ca8-d46c271eb360.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. The previous first nine in-range missing indirect calls at `0x829F6F60..0x829F9DC8` disappeared. The first missing indirect call is now the non-null out-of-range target `0x01000000` at `lr=0x82121160`, followed by the existing `00000000` clusters.

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

1. Trace the `0x01000000` target at `lr=0x82121160`.
   Completion standard: identify the generated function and source field/register path that loads CTR with `0x01000000`.
2. Decide whether `0x01000000` is a malformed function pointer, endian/flagged pointer, import thunk value, or expected nullable/error path.
   Completion standard: choose only after source/register evidence; do not mask the call blindly.
3. Trace the repeated `00000000` cluster around `lr=0x822F983C` and `lr=0x82200F74/0x82200F94`.
   Completion standard: identify at least one guest table slot feeding CTR and whether null dispatch should be skipped or initialized.
4. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either removes/reclassifies the `01000000` missing call or produces a clearly different first blocker.
5. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `0x01000000 @ lr=0x82121160`, still in Windows startup/resource bring-up.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 57: HUD Init Generated Wrapper Restore

Current mainline goal:

- Continue Windows runtime bring-up by replacing stale hand-written wrappers with current ReXGlue generated behavior when source evidence proves the wrapper is corrupting guest state.
- Keep this scoped to the HUD initialization chain and do not start a tracked generated-code refresh.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Traced the non-null out-of-range indirect call `0x01000000 @ lr=0x82121160` to generated `__imp__sub_821A8278`, which loads a HUD object from `0x82CFA2E4`, reads its vtable, then dispatches `vtable[56]`.
- Confirmed the old hand-expanded `sub_821A8868` wrapper skipped current generated initialization side effects before creating/storing the HUD object.
- Confirmed current generated `__imp__sub_82300C78` initializes the object with vtable `0x82010F0C`, runs the generated pre-init path, and stores the object at `0x82CFA2E4`.
- Replaced the old no-op `sub_82300C78` wrapper with a tracing wrapper that calls `__imp__sub_82300C78`.
- Replaced the old hand-expanded `sub_821A8868` wrapper with a tracing wrapper that calls `__imp__sub_821A8868`.
- This keeps the public wrapper instrumentation but restores the current generated initialization sequence for the HUD object and vtable.

Fresh verification:

- Static check reported:
  `GREEN_PASS_HUD_INIT_WRAPPERS_CALL_IMP`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known five `imports.cpp`/`vfs.h` warnings.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-hudimp-7b52dde0-8852-4016-a52b-3fb1e37507e3.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-hudimp-352f1025-a85c-4c6a-a745-8b29cc9cb502.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. The previous `0x01000000` target disappeared. `sub_821A8278` now exits with `r3=0x006084E0`, and the first missing indirect calls are null dispatches: `00000000 @ lr=82300D28`, then `00000000 @ lr=827DB388/827DB3B0/827DB3D0/827DB3F4`, followed by the existing resource/finalization clusters.
- Runtime surface:
  smoke still reaches deeper resource paths including `platform:/textures/hud`, empty `GTA::FileResolve`, embedded DCL probes, `platform:/textures/fx_Rain`, and final setup. This remains startup/resource/runtime bring-up, not playability.

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

1. Trace `00000000 @ lr=82300D28`.
   Completion standard: identify the generated function and vtable/slot that feeds CTR at this call site, and decide whether the null dispatch is expected or missing initialization.
2. Trace the sync null-dispatch cluster at `lr=827DB388`, `827DB3B0`, `827DB3D0`, and `827DB3F4`.
   Completion standard: determine whether these are nullable callbacks inside generated sync setup, missing wrapper-generated side effects, or stale project-side sync hooks.
3. Trace the repeated resource/finalization null cluster around `lr=822F983C` and `lr=82200F74/82200F94`.
   Completion standard: map one repeated cluster to its generated function and source table slot.
4. Add the smallest diagnostic or runtime fix for the first proven root cause.
   Completion standard: no blind masking of null calls; behavior changes require source/register evidence.
5. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either removes/reclassifies the first null-dispatch cluster or produces a clearly different blocker.
6. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at `00000000 @ lr=82300D28`, then the sync null-dispatch cluster at `827DB388..827DB3F4`, still in Windows startup/resource bring-up.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 58: Null-Dispatch Register Diagnostics

Current mainline goal:

- Continue Windows runtime bring-up by distinguishing harmless/null callback dispatch diagnostics from missing initialization or bad function-pointer corruption.
- Keep the diagnostic behavior-preserving: do not skip, synthesize, or install fallback handlers for null indirect calls without source evidence.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Corrected the previous stage note: generated `__imp__sub_82300C78` uses vtable `0x82010F0C`; the old hand-expanded wrapper was wrong because it skipped generated initialization side effects, not because it used a different vtable address.
- Expanded `PPC_CALL_INDIRECT_FUNC` missing-call diagnostics to log `r3`, `r4`, `r5`, `r10`, and `r11` in addition to sequence, target, range classification, `lr`, `ctr`, `r1`, and `r12`.
- This is diagnostic only. It does not change dispatch behavior or guest register state.

Fresh verification:

- Static checks:
  - `git -c core.whitespace=cr-at-eol diff --check -- glue/rexglue-sdk-main/include/rex/ppc/context.h LibertyRecomp/kernel/imports.cpp docs/switch-audit/CONTINUATION_GUIDE.md` passed.
  - `GREEN_PASS_HUD_COMMENT_CORRECTED` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  a full rebuild from the shared ReXGlue context header took a long time and completed in the background after the tool timeout; a fresh follow-up Ninja run reported `ninja: no work to do`, confirming the build finished.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-nullregs-c074b62b-c567-427b-8d7e-02de49c8bc64.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-nullregs-4a0d4498-6a13-4235-8d78-76f9f6b9f862.log`
- Smoke result:
  timed out at the 15-second bound and was killed intentionally. Temporary `portable.txt`, copied `game/default.xex`, and empty `game` directory were removed afterward.

Key evidence:

- `82300D28`: `r3=006084E0`, `r10=FFFFFFFF`, `r11=00000000`, `ctr=00000000`. Source is generated `__imp__sub_82300C78` calling the HUD object's `vtable[4]`; the slot is null and execution continues.
- `827DB388/827DB3B0/827DB3D0/827DB3F4`: `r3=82A80A24`, `r11=00000000`, `ctr=00000000`. Source is generated `__imp__sub_827DB338` calling service-object slots from the fixed object returned by `sub_827E1780` (`0x82A80A24`); the relevant slots are null and execution continues.
- `82121160`: `r3=006084E0`, `r10=82010F0C`, `r11=00000000`, `ctr=00000000`. The earlier bad `0x01000000` value remains gone; this is now also a null HUD vtable slot.
- Repeated resource/finalization clusters remain, especially `822F983C` with `r4=8201052C`, `r5=00000003`, `r10=00000206`, and `r11=00000000`, plus `82200F74/82200F94` with `r10=000001FC`.
- Runtime surface still reaches empty `GTA::FileResolve`, `platform:/textures/hud`, embedded DCL probes, `platform:/textures/fx_Rain`, and final setup. This remains startup/resource/runtime bring-up, not playability.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `glue/rexglue-sdk-main/include/rex/ppc/context.h`,
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

1. Trace the repeated `822F983C` null-dispatch cluster.
   Completion standard: map the generated function, source object/table, and slot value feeding `r11=0`.
2. Trace the paired `82200F74/82200F94` cluster.
   Completion standard: identify why the same call pair repeats with `r10=0x1FC` and whether it is tied to missing resource content or a render setup callback table.
3. Audit empty-path `GTA::FileResolve` success.
   Completion standard: identify at least one caller and decide whether empty path should return failure, root, or a special no-op handle.
4. Add the smallest diagnostic or runtime fix for the first proven root cause.
   Completion standard: no blind masking of null calls; behavior changes require source/register evidence.
5. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either reduces/reclassifies the resource/finalization null-dispatch cluster or produces a clearly different blocker.
6. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at the `822F983C` and `82200F74/82200F94` null-dispatch clusters, still in Windows startup/resource bring-up.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 59: GPU Init Wrapper Restore

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated implementations as the source of truth for project-side strong wrappers.
- Clear startup-chain wrapper recursion without replacing generated behavior with broad hand-written bypasses.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Replaced the old `sub_82856BA8` GPU setup collapse with a tracing wrapper that calls generated `__imp__sub_82856BA8`.
- Confirmed the old bypass left the global GPU/resource manager slot `0x831255F0` uninitialized, feeding later null-dispatch storms around `822F983C` and `82200F74/82200F94`.
- Redirected the local GPU-init subcluster wrappers `sub_829D92C0`, `sub_8286BA28`, `sub_8286CE40`, `sub_82854448`, `sub_8286C8F0`, and `sub_8287E2C0` to their generated `__imp__` implementations after confirming the generated functions exist.
- This is ReXGlue wrapper restoration, not a tracked generated-code refresh and not a gameplay/runtime-completeness claim.

Fresh verification:

- Static checks:
  - `GREEN_PASS_SUB_82856BA8_CALLS_IMP` passed before the later cluster fix.
  - `GREEN_PASS_GPU_INIT_SUBCLUSTER_CALLS_IMP` passed for `sub_829D92C0`, `sub_8286BA28`, and `sub_8286CE40`.
  - `GREEN_PASS_GPU_CLUSTER_CALLS_IMP` passed for `sub_82854448`, `sub_8286C8F0`, and `sub_8287E2C0`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded with the known five `imports.cpp`/`vfs.h` warnings. The existing vcpkg applocal warning about missing `dumpbin`/`objdump` was printed after link and did not fail the build.
- First bounded smoke after enabling generated GPU setup:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-gpusetup-d9fcb511-8568-4aa2-8786-e5395bd7fd73.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-gpusetup-e6c9b8af-44f6-45e8-b381-ab8b8a8bb524.log`
  - result: previous `822F983C` / `82200F74` / `82200F94` missing-null storm disappeared; blocker moved to stack overflow in `sub_829D92C0`.
- Second bounded smoke after fixing `sub_829D92C0`, `sub_8286BA28`, and `sub_8286CE40`:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-gpucluster-b46ca1ba-21b8-40e1-8e9d-d1b5ec079e3d.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-gpucluster-c56c7e1d-8393-49e1-b2a1-bc92c1de662b.log`
  - result: `sub_829D92C0` overflow cleared; blocker moved to `sub_8287E2C0 + 0x55`.
- Final bounded smoke after fixing `sub_82854448`, `sub_8286C8F0`, and `sub_8287E2C0`:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-gpucluster2-final-0ceffb7f-779d-426e-a58f-02d0b7b94c5f.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-gpucluster2-final-7c76caa9-1439-45e7-be07-971edb064f7a.log`
  - result: process exited through the current VEH path with `0x00000000`; do not treat this as success.
  - counters: `MISSING=8`, `INDIRECT=0`, `VEH=1`.
  - runtime surface: reached `[Main] KiSystemStartup done`, loaded `default.xex`, zeroed worker globals, reported entry `0x829A0860`, reached `[Main] Creating video device...`, attempted D3D12/Vulkan backend selection, entered `xstart` after `sub_829A7DC8`, spawned guest-thread workers, then hit a native access violation.
  - graphics evidence: stderr showed `Trying graphics backend: D3D12.`, `Trying graphics backend: Vulkan.`, `volkInitialize failed with error code 0xFFFFFFFD.`, `Graphics interface creation returned null.`, then another `Trying graphics backend: D3D12.` before guest-thread traces continued.
  - first missing calls after guest-thread start were the known nullable/null-dispatch diagnostics at `82300D28`, `827DB388`, `827DB3B0`, `827DB3D0`, `827DB3F4`, `82121160`, `82753D70`, and `824315D4`.
  - current native access violation: `VEH exception code=0xC0000005`, top fault RVA `0x2C9C52`.
  - current map evidence: `0x2C9C52 - 0x1000` maps to `o1heapAllocate + 0x82`; the stack maps through `Heap::Alloc`, `sub_8218BE28`, `sub_8220E108`, `__imp__sub_82120FB8`, `sub_82120FB8`, `__imp__sub_82120000`, `sub_82120000`, `__imp__sub_8218BEB0`, `sub_8218BEB0`, and `GuestThread::Start`.

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

1. Trace the `o1heapAllocate + 0x82` access violation.
   Completion standard: identify the requested allocation size, target heap/arena pointer, and whether the arena is null/corrupt, exhausted, or receiving an invalid alignment/size.
2. Inspect `Heap::Alloc` and the wrappers on the mapped call chain (`sub_8218BE28`, `sub_8220E108`, `sub_82120FB8`, `sub_82120000`, `sub_8218BEB0`).
   Completion standard: determine whether a project-side wrapper is corrupting heap state or whether generated ReXGlue behavior is reaching an uninitialized runtime heap.
3. Keep the graphics backend evidence in view but do not treat the current blocker as purely renderer-side unless heap tracing disproves the mapped stack.
   Completion standard: any graphics change must be tied to heap/guest-thread evidence, not the earlier D3D12/Vulkan log alone.
4. Add the smallest diagnostic or runtime fix for the first proven heap/root-cause boundary.
   Completion standard: no broad allocator rewrite; behavior changes require source/log evidence.
5. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either reaches past the `o1heapAllocate` fault or reports a clearly mapped next blocker.
6. Commit and push the next verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: update this guide, sync it to the old Codex workspace, stage scoped files only, commit, push, then continue.

Next stage entry condition:

- Resume at the `o1heapAllocate + 0x82` access violation reached after module load, graphics backend attempts, `xstart`, and guest-thread startup.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 60: ReXGlue GPU Resource Create Restore

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated implementations as the source of truth for project-side strong wrappers.
- Remove stale hand-written GPU/resource bypasses only when smoke evidence proves they corrupt generated state.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Re-read the ReXGlue wiki/codegen rules for generated weak aliases, strong project overrides, generated source structure, memory, VFS, and CLI/config usage before changing behavior.
- Confirmed the compiled ReXGlue SDK is available at `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest\tools\rexglue-sdk-0.8.1.32-dev.gf22cd9d-win-amd64`.
- Fixed the native memcpy patch linkage to use `PPC_FUNC_IMPL(sub_82990830)` and `PPC_FUNC_IMPL(sub_82990880)`, matching ReXGlue's unmangled generated weak aliases. The previous `PPC_FUNC(...)` definitions produced C++-mangled symbols and did not override the generated public C aliases.
- Instrumented the texture blit path temporarily and traced the bad texture copy arguments back to `__imp__sub_8286BBE8 -> sub_82850028 -> vtable[96] -> sub_8286ABF0`.
- Proved the old `sub_82850028` wrapper returned success without calling the generated body, leaving the caller's output surface slot at `dstObj+24` unset. Before the fix, `sub_8286ABF0` entered with valid texture object dimensions but `dstSurface=0`, causing null/huge native memcpy calls under `sub_829E5110`.
- Replaced the old `sub_82850028` success bypass with a tracing wrapper that calls generated `__imp__sub_82850028`.
- Confirmed the fix makes generated resource creation write real surface pointers such as `0x80177D40`, `0x80178210`, and `0x80178C20`, and `sub_829E5110` then receives sane width/height/format arguments.
- Fixed the next wrapper-recursion blocker: `sub_8285F6C0` now declares and calls generated `__imp__sub_8285F6C0` instead of recursively calling the public alias.
- Removed the temporary `TEXFMT-WATCH`, `TEXBLIT-WATCH`, and `MEMCPY-WATCH` hooks before final verification. The committed behavior is the ReXGlue override/linkage fix plus wrapper restoration, not diagnostic masking.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp LibertyRecomp/patches/memcpy_patches.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remain: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, and the existing `ctx.lr` printf format warning. The existing vcpkg applocal warning about missing `dumpbin`/`objdump` still appears after link and does not fail Ninja.
- Final clean bounded smoke used a temporary `portable.txt` and a temporary `game` junction pointing to:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`
- Final clean bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-clean-gpucreate-1824555d-5eb2-460a-a07f-91f976acf7dc.log`
- Final clean bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-clean-gpucreate-6382829e-1288-4862-8b12-e3c0cc1abf40.log`
- Smoke result:
  the process was killed intentionally at the 35-second bound. There were `0` VEH entries, `0` missing indirect calls, `0` temporary watch logs, and no old null/huge memcpy diagnostics because the watch hooks were removed. The temporary `game` junction and `portable.txt` were removed afterward.
- Runtime surface:
  reached module load, host video creation, guest-thread starts, texture/resource work, and VFS/file-resolution calls. It still does not represent playability.
- Current front edge:
  real-directory smoke still reports missing resolved paths such as `platform:/textures/fonts`, `platform:/textures/buttons_360`, `platform:/textures/hud`, `platform:/textures/skydome`, and `platform:/textures/fx_Rain`, plus repeated PPC `tw/td trap hit (type 22)` warnings. `common:/DATA/LOADINGSCREENS_360.DAT` also goes through the current `\Device\Harddisk0\partition0` resolution path and remains unresolved in the sampled log.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `LibertyRecomp/patches/memcpy_patches.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Trace the `platform:/textures/*` VFS misses with the real extracted directory.
   Completion standard: identify whether those paths should resolve to extracted files, RPF archive contents, or a generated/embedded fallback.
2. Trace `common:/DATA/LOADINGSCREENS_360.DAT` through `\Device\Harddisk0\partition0`.
   Completion standard: map the current root-device normalization path and decide whether the VFS should translate this device prefix before file lookup.
3. Trace repeated `tw/td trap hit (type 22)` warnings.
   Completion standard: identify one generated function and guest condition producing the repeated trap, then decide whether it is expected polling/assert behavior or a real blocker.
4. Keep the next behavior change ReXGlue-first.
   Completion standard: if a project-side wrapper is involved, confirm the generated `__imp__` implementation and current wrapper behavior before editing; if VFS is involved, use existing VFS/root mapping APIs rather than ad hoc string hacks.
5. Rebuild and run bounded smoke after the next minimal change.
   Completion standard: Windows build succeeds and smoke either resolves/reclassifies the first resource path blocker or produces a clearly mapped next blocker.
6. Commit and push the verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: stage only scoped files, commit, push, and leave unrelated dirty entries untouched.

Next stage entry condition:

- Resume at Windows VFS/resource path bring-up after the ReXGlue GPU resource create restoration. Start with `platform:/textures/*`, `common:/DATA/LOADINGSCREENS_360.DAT`, and the repeated trap warnings.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 61: ReXGlue VFS Content Preflight

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated code and ReXGlue's VFS model as the reference.
- Do not treat missing `platform:/textures/*` resources as wrapper bugs until the content layout and RPF extraction preconditions are proven.
- Keep Switch frozen as the verified ExeFS/NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.

Completed in this batch:

- Re-read ReXGlue `Virtual-File-System.md` and `Generated-Code-Structure.md`.
- Reconfirmed the built ReXGlue CLI is callable:
  `rexglue.exe --version` returned `0.8.1.32-dev.gf22cd9d`, and `rexglue codegen --help` reported the codegen command.
- Checked the existing `RpfLoader` integration and found no production call sites for `RpfLoader::Initialize`, `ScanForRpfFiles`, `HasFile`, or `ExtractFile`; it is not currently serving VFS misses.
- Checked the real smoke content directory:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`.
  It has top-level `common.rpf`, `xbox360.rpf`, and `audio.rpf`, plus `xbox360/audio`, but it does not have `xbox360/textures`.
- Read the reference `D:\GTA4 NS\LibertyRecomp-main\docs\RPF_EXTRACTION_DESIGN.md` and current installer code. The intended project model is install/preparation-time RPF extraction into `game/common`, `game/xbox360`, and `game/audio`, then VFS direct file serving, not relying on raw top-level RPF archives during normal runtime.
- Confirmed the current repo has no `aes_key.bin` in the build output or expected bundled paths. The actual RPF name table bytes sampled from `common.rpf`, `xbox360.rpf`, and `audio.rpf` are not readable without the AES key/extraction path.
- Added Windows-only startup preflight logging in `LibertyRecomp/main.cpp` so smoke logs now state whether the expected direct-VFS content exists:
  `default.xex`, extracted `common`, extracted `xbox360`, extracted `xbox360/textures`, extracted `audio`, `xbox360/audio`, source RPF archives, and AES key paths.
- Added a Windows `audio:` fallback matching the existing Switch audit behavior: use `game/audio` if present, otherwise use `game/xbox360/audio` when present.

Fresh verification:

- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. Existing `vfs.h` block-comment warning remains. The existing vcpkg applocal warning about missing `dumpbin`/`objdump` still appears after link and does not fail Ninja.
- Final bounded smoke used a temporary `portable.txt` and a temporary `game` junction pointing to:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-vfs-preflight-stdout-931b36ee-6212-4174-8922-c602b13f0a12.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-vfs-preflight-stdout-e374bc42-fa70-4f0f-a4e7-e7ca22141117.log`
- Smoke result:
  the process was killed intentionally at the 35-second bound. There were `0` VEH entries, `11` VFS missing entries, `0` VFS found entries, `12` visible Windows VFS preflight lines, and `11` stderr `MISSING-FUNC` diagnostics. The temporary `game` junction and `portable.txt` were removed afterward.
- Preflight evidence:
  `default.xex`, extracted `common`, extracted `xbox360`, `xbox360/audio`, and the three source RPF archives are present. `xbox360/textures`, top-level `audio`, install `aes_key.bin`, and bundled `aes_key.bin` are missing.
- Runtime surface:
  reached module load, graphics backend attempts, guest-thread startup, file-resolution calls, and resource path requests without a VEH inside the 35-second smoke bound. This still does not represent playability.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/main.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Decide the next content-path boundary from evidence, not guesses.
   Completion standard: either provide/locate `aes_key.bin` and run installer/RPF extraction into a complete direct-VFS layout, or explicitly keep raw RPF runtime access out of scope for this phase.
2. If a complete extracted layout becomes available, rerun the same bounded smoke.
   Completion standard: `platform:/textures/fonts`, `platform:/textures/buttons_360`, `platform:/textures/hud`, `platform:/textures/skydome`, and `platform:/textures/fx_Rain` are either resolved or replaced by a clearly mapped later blocker.
3. Trace the current stderr `MISSING-FUNC` diagnostics after the content layout boundary is stable.
   Completion standard: map the first LR/R12 pair to generated/source functions and decide whether it is expected nullable callback dispatch or a missing vtable/import setup.
4. Trace `common:/DATA/LOADINGSCREENS_360.DAT` and the current `\Device\Harddisk0\partition0` miss against the ReXGlue VFS null-device model.
   Completion standard: decide whether this project VFS needs a ReXGlue-style null device/no-op path for Partition0 or whether the path should be translated to `common:`.
5. Keep the next behavior change ReXGlue-first.
   Completion standard: use generated `__imp__` implementations for wrapper hooks, ReXGlue VFS behavior for root-device decisions, and existing installer/RPF extractor paths for content prep.
6. Rebuild, smoke, update this guide, stage scoped files only, commit, push, and continue.

Next stage entry condition:

- Resume at Windows VFS/content-layout stabilization with visible preflight evidence. The highest-value next step is to obtain or generate the complete extracted `game/xbox360/textures` layout before chasing resource-path misses as runtime bugs.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 62: ReXGlue NullDevice VFS Boundary

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated implementations and ReXGlue runtime behavior as the reference.
- Keep Switch frozen as the verified LibertyRecompExeFs / NSP-like pre-guest baseline unless a scoped Switch regression is explicitly required.
- Do not treat nullable guest vtable dispatch diagnostics as blockers without source/register evidence that the null is corrupt state.

Completed in this batch:

- Re-read the current audit guide, ReXGlue Function Overrides / Generated Code Structure / Virtual File System wiki pages, and confirmed the built ReXGlue CLI reports `0.8.1.32-dev.gf22cd9d`.
- Corrected `sub_82300C78` from `PPC_FUNC` to `PPC_FUNC_IMPL` so the existing tracing wrapper actually overrides the generated weak public alias while still delegating to generated `__imp__sub_82300C78`.
- Verified the link map now resolves public `sub_82300C78` to `imports.cpp.obj` and keeps `__imp__sub_82300C78` in `LibertyRecompLib:gta4_recomp.12.cpp.obj`.
- Confirmed the old `sub_827DB338` bypass remains inactive; public `sub_827DB338` still resolves to generated `gta4_recomp.52.cpp.obj`. Do not enable that stale non-blocking bypass without a fresh root-cause trace.
- Reclassified the fresh `82300D28`, `827DB388..827DB3F4`, and `82121160` missing-function entries as null guest vtable/service slots that are logged and then continue, matching the earlier null-dispatch diagnostic stage.
- Implemented a ReXGlue-style VFS null-device classifier for `\Device\Harddisk0\Partition0`, `\Device\Harddisk0\Cache0`, and `\Device\Harddisk0\Cache1`.
- `VFS::Exists()` now treats those paths as successful zero-byte virtual entries; `VFS::GetFileSize()` returns `0`; `VFS::Resolve()` logs `[VFS] NULL DEVICE` and does not return a fake host filesystem path.
- This aligns the legacy LibertyRecomp VFS with ReXGlue's NullDevice model without creating a fake host file or changing raw RPF/content extraction policy.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp LibertyRecomp/kernel/vfs.h LibertyRecomp/kernel/vfs.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remain: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, existing `ctx.lr` printf format warning, and the existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- First bounded smoke after the `sub_82300C78` ABI correction:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-sub82300-dd3e9036-9613-4e44-9c5e-31b06e1d9402.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-sub82300-99516b9e-ac2a-408a-a31b-865e018c2cbf.log`
  - result: killed intentionally at the 35-second bound, `SUB82300_COUNT=2`, `MISSING_FUNC_COUNT=11`, `VEH_COUNT=0`.
- Final bounded smoke after the VFS NullDevice change:
  - stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-nulldevice-954a5ba1-ac34-475b-8949-aa4ebf86db8b.log`
  - stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-nulldevice-423395bb-c57c-4e22-a9c3-096d1d886092.log`
  - result: killed intentionally at the 35-second bound, `NULL_DEVICE_COUNT=1`, `PARTITION0_NOT_FOUND_COUNT=0`, `SUB82300_COUNT=2`, `MISSING_FUNC_COUNT=11`, `VEH_COUNT=0`.
- Temporary `portable.txt` and the temporary `game` junction to `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)` were removed after each smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `LibertyRecomp/kernel/vfs.cpp`,
  `LibertyRecomp/kernel/vfs.h`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`,
  `.planning/`,
  `docs/dev/`.

Next small tasks:

1. Continue VFS/content-layout stabilization from the verified NullDevice boundary.
   Completion standard: do not revisit `Partition0` as a normal missing file unless a new log contradicts the NullDevice classification.
2. Trace the remaining `platform:/textures/*` misses against the real content layout and RPF extraction requirements.
   Completion standard: identify whether `fonts`, `buttons_360`, `hud`, `skydome`, and `fx_Rain` require a completed extracted `xbox360/textures` tree, raw RPF extraction, or a specific embedded fallback.
3. Trace `common:/DATA/LOADINGSCREENS_360.DAT` separately from the raw HDD null-device path.
   Completion standard: map whether this should resolve through `common/` after extraction or whether the current wrapper returns before using the VFS result.
4. Keep wrapper changes ReXGlue-first.
   Completion standard: if another project-side wrapper is involved, confirm the generated `__imp__` implementation and link map ownership before editing.
5. Rebuild and run a bounded smoke after the next minimal behavior change.
   Completion standard: Windows build succeeds and smoke either resolves/reclassifies one content path blocker or produces a clearly mapped next blocker.
6. Commit and push the verified batch with `D:\Git\cmd\git.exe`, then continue immediately.
   Completion standard: stage only scoped files, commit, push, and leave unrelated dirty entries untouched.

Next stage entry condition:

- Resume at Windows VFS/content-layout bring-up after the NullDevice boundary. The active blockers are incomplete `xbox360/textures` extraction / raw RPF extraction preconditions and `common:/DATA/LOADINGSCREENS_360.DAT` resolution.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 63: Platform Texture Content Layout Diagnostic

Current mainline goal:

- Continue Windows runtime bring-up with ReXGlue VFS behavior as the reference.
- Keep Switch frozen at the LibertyRecompExeFs / NSP-like pre-guest baseline unless a scoped regression check is required.
- Do not chase `platform:/textures/*` misses as wrapper bugs while the content layout still lacks the extracted `game/xbox360/textures` tree and AES key.

Completed in this batch:

- Re-read the current audit guide, ReXGlue `Virtual-File-System.md`, and ReXGlue `Generated-Code-Structure.md`.
- Reproduced the current Windows VFS/content front edge with the correct portable setup under the executable directory:
  `...\liberty-build-x64-clang-nomanifest\LibertyRecomp\portable.txt` and a temporary `game` junction to the user's real GTA IV directory.
- Confirmed the previous failed smoke setup was invalid because it placed `portable.txt` and `game` in the build root instead of the executable root, sending the app through installer/video setup instead of guest startup.
- Confirmed the real content preflight sees `default.xex`, extracted `common`, extracted `xbox360`, `xbox360/audio`, and the three source RPF archives, but does not see `xbox360/textures`, top-level `audio`, install `aes_key.bin`, or bundled `aes_key.bin`.
- Checked raw source RPF headers:
  - `common.rpf`: `RPF2`, encrypted `0xFFFFFFFF`
  - `xbox360.rpf`: `RPF2`, encrypted `0xFFFFFFFF`
  - `audio.rpf`: `RPF2`, encrypted `0xFFFFFFFF`
- Searched the current repo and `D:\GTA4 NS` for `aes_key.bin`; none was found.
- Confirmed `RpfLoader` exists but is not currently wired into `VFS::Resolve()`, and current source RPFs cannot be parsed usefully without AES key material anyway.
- Added a one-shot VFS diagnostic for missing platform texture content. When `platform:/textures/*` misses and `game/xbox360/textures` is absent, `VFS::Resolve()` now logs:
  `[VFS] CONTENT LAYOUT MISSING: platform texture request ... (xbox360.rpf=present aes_key.bin=missing)`.
- This is a diagnostic boundary only. It does not synthesize files, does not alter path resolution, and does not enable raw RPF runtime extraction.

Fresh verification:

- Red check before the change:
  `CONTENT_LAYOUT_DIAGNOSTIC_COUNT=0` and the assertion for the new diagnostic failed as expected.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/vfs.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. Existing warning remained: `vfs.h` block-comment warning. The existing vcpkg applocal warning about missing `dumpbin` / `objdump` still appears after link and does not fail Ninja.
- Final bounded smoke used a temporary executable-root `portable.txt` and executable-root `game` junction pointing to:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`.
- Final bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-contentdiag-27dc1e3b-7a69-4f27-b6ad-4de09d948047.log`
- Final bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-contentdiag-b7591408-d055-46ce-ab42-8fbf8be226a7.log`
- Smoke result:
  killed intentionally at the 45-second bound. Counts: `Windows VFS preflight=12`, `CONTENT LAYOUT MISSING=1`, `platform:/textures=26`, `LOADINGSCREENS=1`, `NULL_DEVICE=1`, `MISSING-FUNC=11`, `tw/td trap hit=99`.
- A precise exception keyword scan found no `[VEH]`, access violation, unhandled exception, `0xC0000005`, or `0xC00000FD` lines. The earlier broad `VEH` count was a false positive from text such as `vehicleFx`.
- Temporary `portable.txt` and the temporary `game` junction were removed after the smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/vfs.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`.

Next small tasks:

1. Decide the next content-prep boundary.
   Completion standard: either locate/generate a valid `aes_key.bin` and run installer/RPF extraction toward `game/xbox360/textures`, or explicitly keep raw encrypted RPF extraction out of scope for the next runtime step.
2. If content remains incomplete, stop chasing `platform:/textures/*` as a runtime VFS bug.
   Completion standard: use the new diagnostic as evidence and move to a blocker that can be fixed without missing content, such as the repeated trap or nullable vtable diagnostics.
3. Trace one repeated `tw/td trap hit (type 22)` source.
   Completion standard: map one repeated LR/R12/function source and decide whether it is expected assert/polling behavior or a real runtime blocker.
4. Trace the newer `MISSING-FUNC` entries after the known null slots.
   Completion standard: map `82753D70`, `824315D4`, or `82196C50/98/B4` to generated/source context and decide whether a wrapper, vtable setup, or service callback is missing.
5. Keep any next wrapper/runtime change ReXGlue-first.
   Completion standard: confirm generated `__imp__` ownership and link-map behavior before editing wrappers; use ReXGlue VFS semantics for device/path behavior.
6. Rebuild, bounded smoke, update this guide, stage scoped files only, commit, push, and continue immediately.

Next stage entry condition:

- Resume after the verified platform texture content diagnostic. The immediate choice is content prep (`aes_key.bin` plus RPF extraction) versus moving to the repeated trap / newer `MISSING-FUNC` runtime diagnostics while content remains incomplete.
- Do not switch to Switch build work unless Windows changes require a scoped Switch regression check or the user explicitly redirects.

## 2026-06-18 Windows Continuation 64: ReXGlue Trap Context Diagnostic

Current mainline goal:

- Continue Windows runtime bring-up with ReXGlue generated `__imp__` implementations as the reference.
- Keep the Switch side frozen at the LibertyRecompExeFs / NSP-like pre-guest baseline unless a scoped regression check is explicitly needed.
- Do not treat repeated `tw/td trap hit (type 22)` lines as a crash without mapping their LR/CTR/R12 source first.

Completed in this batch:

- Re-read the ReXGlue `Function-Overrides.md` and `Generated-Code-Structure.md` wiki pages.
- Confirmed the generated code pattern: public recompiled functions are weak aliases, `__imp__*` symbols are the generated implementations, and project-side strong public symbols are the correct wrapper/override boundary.
- Added bounded ReXGlue PPC trap context diagnostics in `glue/rexglue-sdk-main/include/rex/ppc/context.h`.
- `ppc_trap()` now logs `lr`, `ctr`, `r12`, `r1`, `r3`, `r4`, and a monotonic count for trap types `0` and `22`.
- The trap log is rate-limited to the first 128 hits and every 1000th hit afterward so a polling/assert loop does not flood logs indefinitely.
- Mapped the repeated trap source:
  - trap LR: `0x82994870`
  - `ctr`: `0x82A0270C`
  - `r12`: `0x8298EF88`
  - `r3`: `0x00000002`
  - `r4`: `0x00000000`
- Generated-code mapping:
  - `0x82994870` is inside `__imp__sub_82994840` in `gta4_recomp.66.cpp`.
  - `sub_82994840` loads the callback/global at `0x83010000 - 31808 = 0x830083C0`; when it is zero, it calls `sub_8299BEA0` with `r3=2` and then executes `twi 31,r0,22`.
  - `0x8298EF88` is a caller in `gta4_recomp.65.cpp` that stores trap code `22` and calls `sub_82994840`.
  - `0x82A0270C` also appears as the generated `KeTlsGetValue` import thunk / disabled BootGlobals `VTABLE2_VALUE`, so it must not be blindly installed as an assert callback without checking the guest initialization path.
- Confirmed `sub_82992680` calls `sub_82994830(0)`, and generated `__imp__sub_82994830` stores its `r3` argument into `0x830083C0`. The currently observed trap is therefore consistent with the guest failure/assert callback pointer being intentionally zeroed or not later initialized.
- No generated code was edited.

Fresh verification:

- Red check before the diagnostic change:
  `TRAP_CONTEXT_DIAGNOSTIC_COUNT=0` for the expected `tw/td trap hit (type 22 lr=` needle, confirming the old logs did not expose the necessary context.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- glue/rexglue-sdk-main/include/rex/ppc/context.h` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. The first Ninja invocation timed out at the Codex command bound while the background build continued; a follow-up Ninja invocation returned `ninja: no work to do`, confirming the target completed.
- Bounded smoke used a temporary executable-root `portable.txt` and executable-root `game` junction pointing to:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-trapctx-3e6acd66-1dd0-45bb-8d54-5c0def5d8a90.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-trapctx-6577b2fe-3a6f-418a-8594-55413b441b46.log`
- Smoke result:
  killed intentionally at the 45-second bound. Counts: `tw/td trap hit (type 22 lr=)=99`, `[VFS] CONTENT LAYOUT MISSING=1`, `MISSING-FUNC=11`, `[VEH]=0`, `0xC0000005=0`, `0xC00000FD=0`.
- Fresh pre-commit verification rerun:
  - Build: `ninja: no work to do` with exit code 0.
  - Smoke stdout: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-trapctx-fresh-3879ad9b-9dc6-44a8-b71c-4810d831606d.log`
  - Smoke stderr: `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-trapctx-fresh-8ea88450-769b-4e13-99cd-f526840472d3.log`
  - Smoke result: killed intentionally at the 45-second bound. Counts: `TRAP_CONTEXT_COUNT=99`, `CONTENT_LAYOUT_MISSING_COUNT=1`, `MISSING_FUNC_COUNT=11`, `VEH_COUNT=0`, `ACCESS_VIOLATION_COUNT=0`, `UNHANDLED_EXCEPTION_COUNT=0`, `C0000005_COUNT=0`, `C00000FD_COUNT=0`.
  - First trap line: `lr=82994870 ctr=82A0270C r12=8298EF88 r1=800802E0 r3=00000002 r4=00000000 count=1`.
- Temporary `portable.txt` and the temporary `game` junction were removed after the smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `glue/rexglue-sdk-main/include/rex/ppc/context.h`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope if they reappear in a full status:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`.

Next small tasks:

1. Add a scoped ReXGlue-style wrapper diagnostic around `sub_82994830` / `sub_82994840` only if the next smoke needs setter/read evidence.
   Completion standard: confirm public wrapper ownership with `__imp__` call-through and do not edit generated files.
2. Determine whether `0x830083C0` is expected to remain zero after CRT init.
   Completion standard: map every generated writer/caller of `sub_82994830` and the boot call order that reaches `sub_82992680`.
3. If the assert callback is intentionally zero, reclassify repeated type-22 traps as expected guest assert/reporting noise and move to newer `MISSING-FUNC` entries.
   Completion standard: document the evidence and stop treating the type-22 trap count as the primary blocker.
4. If a missing initialization path should install a callback, implement the smallest project-side wrapper/initialization fix.
   Completion standard: use a strong public wrapper plus `__imp__` call-through, build, smoke, and verify the callback value before and after the change.
5. Continue to the next runtime blocker after this diagnostic is committed.
   Completion standard: build succeeds, bounded smoke has no precise exception keywords, this guide is updated, scoped files are committed and pushed.

Next stage entry condition:

- Resume at the `0x830083C0` assert callback boundary, not at generic `tw/td trap` logging.
- The next decision is whether to instrument `sub_82994830` / `sub_82994840`, or reclassify type-22 traps as expected once boot-order evidence proves the zero callback is intentional.

## 2026-06-18 Windows Continuation 65: Assert Callback Wrapper Evidence

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated `__imp__` functions as the call-through reference.
- Keep wrapper diagnostics narrow and evidence-oriented; do not replace generated code or install synthetic callbacks without proving the guest expects one.
- Treat the repeated type-22 trap as a mapped guest assert/reporting path unless new evidence shows it blocks progress.

Completed in this batch:

- Added project-side wrapper diagnostics for `sub_82994830` and `sub_82994840` in `LibertyRecomp/kernel/imports.cpp`.
- The wrappers call through to generated `__imp__sub_82994830` / `__imp__sub_82994840` and only log the assert callback global at `0x830083C0`.
- Confirmed the correct override macro for this codebase is `PPC_FUNC_IMPL(name)`, matching the existing `sub_82300C78` wrapper. A first attempt using `PPC_FUNC(name)` compiled but produced a C++ mangled symbol and did not override the generated weak public alias.
- Link-map ownership after the correction:
  - public `sub_82994830`: `imports.cpp.obj`
  - public `sub_82994840`: `imports.cpp.obj`
  - `__imp__sub_82994830`: `LibertyRecompLib:gta4_recomp.66.cpp.obj`
  - `__imp__sub_82994840`: `LibertyRecompLib:gta4_recomp.66.cpp.obj`
- Smoke evidence:
  - `sub_82994830` is called once from `lr=0x829926A4`.
  - It requests `0x00000000` and leaves `0x830083C0` as `0x00000000`.
  - `sub_82994840` then repeatedly enters from `lr=0x8298EF88`, reads callback `0x00000000`, calls the generated implementation, and returns with `r3=0x00000002`.
- This proves the current repeated type-22 trap is not caused by a missing host-installed assert callback. The guest CRT path explicitly zeroes the callback via `sub_82992680 -> sub_82994830(0)`.
- No generated code was edited.

Fresh verification:

- Red check before the wrapper diagnostic:
  previous smoke logs had `ASSERT_CALLBACK_DIAGNOSTIC_COUNT=0`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remained: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, existing `ctx.lr` printf format warning, and the existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- Link-map check:
  `LibertyRecomp.map` resolved public `sub_82994830` and public `sub_82994840` to `imports.cpp.obj`, while the `__imp__` symbols remained in `gta4_recomp.66.cpp.obj`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-assertcb-impl-95ceed32-ac30-458f-b1cc-0e2edd8a3816.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-assertcb-impl-71dbcfb7-39c5-4603-8eda-9ead58a0bf35.log`
- Smoke result:
  killed intentionally at the 45-second bound. Counts: `ASSERTCB_TOTAL=33`, `ASSERTCB_SETTER=1`, `ASSERTCB_READER_ENTER=16`, `TRAP_CONTEXT_COUNT=99`, `CONTENT_LAYOUT_MISSING_COUNT=1`, `MISSING_FUNC_COUNT=11`, `VEH_COUNT=0`, `ACCESS_VIOLATION_COUNT=0`, `UNHANDLED_EXCEPTION_COUNT=0`, `C0000005_COUNT=0`, `C00000FD_COUNT=0`.
- Temporary `portable.txt` and the temporary `game` junction were removed after the smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope if they reappear in a full status:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`.

Next small tasks:

1. Stop treating `tw/td trap hit (type 22)` from `lr=82994870 / r12=8298EF88` as the primary blocker.
   Completion standard: continue to log it as guest assert/reporting evidence, but prioritize newer `MISSING-FUNC` entries and content-layout blockers.
2. Map the newest actionable `MISSING-FUNC` entries.
   Completion standard: trace `82753D70`, `824315D4`, and `82196C50/98/B4` to generated source and classify each as nullable callback, vtable setup gap, or missing service wrapper.
3. Keep wrapper changes ReXGlue-first.
   Completion standard: for any new wrapper, use `PPC_FUNC_IMPL` / strong public symbol and confirm map ownership before smoke.
4. If a `MISSING-FUNC` is nullable/non-blocking, document it and move on.
   Completion standard: smoke evidence shows execution continues and no precise exception keywords appear.
5. If a `MISSING-FUNC` is a real missing service callback, implement the smallest wrapper or initializer that makes the generated call path more faithful.
   Completion standard: build, map check if applicable, bounded smoke, updated guide, scoped commit, push.

Next stage entry condition:

- Resume at newer `MISSING-FUNC` mapping, starting with `lr=82753D70` or `lr=824315D4`.
- Keep the assert callback diagnostics in place for now; remove or lower them only after the next runtime blocker no longer needs this evidence.

## 2026-06-18 Windows Continuation 66: Missing-Function VTable Slot Snapshot

Current mainline goal:

- Continue Windows runtime bring-up using ReXGlue generated code and runtime behavior as the primary reference.
- Keep the Switch side frozen at the verified LibertyRecompExeFs / NSP-like pre-guest baseline unless a scoped regression check is explicitly required.
- Do not treat every `MISSING-FUNC` as a missing wrapper. First prove whether the indirect target is a generated function pointer, a nullable callback, or guest data/vtable state.

Completed in this batch:

- Extended the existing `PPC_CALL_INDIRECT_FUNC` diagnostic in `glue/rexglue-sdk-main/include/rex/ppc/context.h`.
- The missing-indirect log now snapshots:
  - `r3_vtbl = *(uint32_t*)r3` when readable,
  - `r3_vtbl` slots at offsets `0`, `4`, `8`, `16`, and `32`,
  - `r11` slots at offsets `0`, `4`, and `8`.
- The diagnostic is bounded to the existing first `256` missing-indirect log entries and does not change dispatch behavior or guest register state.
- Fresh smoke shows all `11` current `MISSING-FUNC` entries are zero guest vtable/service slots rather than isolated missing host wrappers:
  - `lr=82300D28`, `r3=0060A960`, `r3_vtbl=82010F0C`, sampled slots all zero.
  - `lr=827DB388/827DB3B0/827DB3D0/827DB3F4`, `r3=82A80A24`, `r3_vtbl=8200B62C`, sampled slots all zero.
  - `lr=82121160`, `r3=0060A960`, `r3_vtbl=82010F0C`, sampled slots all zero.
  - `lr=82753D70`, `r3=02063420`, `r3_vtbl=8201AA50`, sampled slots all zero.
  - `lr=824315D4`, `r3=0259D520`, `r3_vtbl=82018F6C`, sampled slots all zero.
  - `lr=82196C50/82196C98/82196CB4`, `r3=03079DA0`, `r3_vtbl=82001488`, sampled slots all zero.
- Source mapping before this note:
  - `82753D70` is inside generated `__imp__sub_82753D30`, which loads an object vtable and calls slot `+4`.
  - `824315D4` follows an allocation/constructor path where generated `__imp__sub_823F46C0` stores vtable `0x82018F6C`, then the caller dispatches slot `+4`.
  - `82196C50/82196C98/82196CB4` are inside generated `gta4_recomp.3.cpp` object setup using vtable `0x82001488`, followed by slot calls.
- Current interpretation: the active `MISSING-FUNC` cluster is most likely a vtable/data restoration or image data initialization boundary, not a batch of unrelated wrapper-recursion bugs.
- ReXGlue codegen documentation says the VTable scanner discovers slots from RTTI COL structures and registers slot addresses as `VTABLE` authority functions. The runtime evidence here is different: the object points at plausible image vtable addresses, but the sampled slots in guest memory are zero at runtime.
- No generated code was edited.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- glue/rexglue-sdk-main/include/rex/ppc/context.h` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 2 LibertyRecomp`
- Build result:
  initial header-change rebuild exceeded the command timeout while Ninja kept running; after the background build finished, a follow-up Ninja run returned `ninja: no work to do` with exit code `0`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-missing-vtbl-fresh-8af8b2ab-b7e6-4f0d-9ec9-d88e05bba2d0.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-missing-vtbl-fresh-8af8b2ab-b7e6-4f0d-9ec9-d88e05bba2d0.log`
- Smoke result:
  killed intentionally at the bounded smoke timeout. Counts: `MISSING_FUNC_COUNT=11`, `MISSING_FUNC_VTBL_FIELD_COUNT=11`, `ASSERTCB_TOTAL=0`, `TRAP_CONTEXT_COUNT=0`, `CONTENT_LAYOUT_MISSING_COUNT=1`, `VEH_COUNT=0`, `ACCESS_VIOLATION_COUNT=0`, `UNHANDLED_EXCEPTION_COUNT=0`, `C0000005_COUNT=0`, `C00000FD_COUNT=0`.
- Temporary `portable.txt` and the temporary executable-root `game` junction were removed after the smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `glue/rexglue-sdk-main/include/rex/ppc/context.h`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `docs/dev/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Inspect ReXGlue codegen/runtime vtable restoration and image-data initialization.
   Completion standard: identify where vtable words at `0x82010F0C`, `0x8200B62C`, `0x8201AA50`, `0x82018F6C`, and `0x82001488` should be populated in the current Windows runtime.
2. Compare current generated output and ReXGlue wiki behavior.
   Completion standard: decide whether the current generated data omits these slot words, whether the image blob is not being copied into guest memory, or whether these zero slots are intentional nullable/pure-virtual slots.
3. Use IDA MCP or batch IDA only if generated/source evidence cannot prove the original XEX data words.
   Completion standard: if used, verify the original XEX or loaded image bytes for at least one vtable address and record the exact source of truth.
4. Keep wrapper changes ReXGlue-first.
   Completion standard: if a wrapper turns out to own a missing object construction side effect, confirm public wrapper ownership and `__imp__` call-through before editing.
5. Rebuild, bounded smoke, update this guide, stage scoped files only, commit, push, and continue.
   Completion standard: fresh build succeeds, smoke has no precise exception keywords, and the next runtime blocker is classified with source/register evidence.

Next stage entry condition:

- Resume at ReXGlue vtable/data restoration investigation, not at generic `MISSING-FUNC` counting.
- The next decision is whether the zero vtable slots are expected guest null slots, missing generated vtable entries, or missing image/data materialization in the legacy Windows bootstrap.

## 2026-06-18 Windows Continuation 67: VTable Clear Source Isolated

Current mainline goal:

- Continue Windows runtime bring-up first, with ReXGlue generated code and runtime behavior as the source of truth.
- Keep Switch frozen at LibertyRecompExeFs / NSP-like pre-guest baseline unless explicitly testing Switch.
- Do not count `MISSING-FUNC` entries as wrapper work until guest memory/data mutation has been proven.

Completed in this batch:

- Proved the watched vtable words are nonzero in the original GTA IV `default.xex` image:
  - `0x82010F0C`, `0x8200B62C`, `0x8201AA50`, `0x82018F6C`, and `0x82001488` all contain nonzero words in the decrypted/expanded XEX image.
- Added bounded diagnostics around loader and boot boundaries:
  - `LdrLoadModule after image copy` and `LdrLoadModule after side effects` still show nonzero vtable words.
  - `sub_829A7EA8` enter/exit still shows nonzero vtable words.
  - `sub_829A7DC8` enter/exit still shows nonzero vtable words.
  - `sub_8218BEA8 wrapper enter` still shows nonzero vtable words.
- Added write-side instrumentation:
  - ReXGlue `PPC_STORE_*` / `PPC_MM_STORE_*` watched-store logging.
  - Liberty `rexcrt_memcpy`, `rexcrt_memset`, and `rexcrt_XMemCpy` watched-native-write logging.
  - Smoke showed `VTBL_STORE_COUNT=0` and `VTBL_NATIVE_WRITE_COUNT=0`, so the clear bypassed both paths.
- Added Windows page-protection watchpoint on the watched vtable pages, armed after `sub_8218BE28 exit #819`.
- Fresh smoke caught the first write:
  - `first-zero stage=sub_8218BE28 enter #847`.
  - `[VTBL-WATCH] first write fault guest=0x82001000`.
  - Faulting native PC resolved to `libvcruntime:memset`.
  - Stack return address `rva=0x00067AF6` mapped via `LibertyRecomp.map` to `sub_822C1A30` in `imports.cpp.obj`.
  - Disassembly around `0x140067add` shows `sub_822C1A30` calling `memset(base + 0x82000000, 0, 0x20000)` and returning at `0x140067af6`.
- Current root cause:
  - The synthetic stream initialization wrapper `sub_822C1A30` clears `0x82000000-0x82020000`.
  - That range is not a safe synthetic stream pool in the loaded GTA IV image; it contains live XEX data/vtables used after `sub_8218BEA8` starts.
  - The clear turns the vtable words into zero and causes the current 11-entry `MISSING-FUNC` cluster.
- Reference recorded from OZORDI/XenonRecomp commit `207253d67cdef67235805d595999fa0a2e4fcbd9` (`fix/memory-leak-static-variables`):
  - Commit title: `Fix memory leak: remove static from local variables in Recompile()`.
  - It removes `static` from `std::unordered_set<size_t> labels`, `std::string tempString`, and `std::vector<uint8_t> temp` in XenonRecomp `recompiler.cpp`.
  - Reason: static local containers persist across function recompiles and keep growing during GTA IV-scale recompilation, causing unbounded memory use.
  - Future use: when running or adapting XenonRecomp/ReXGlue codegen for GTA IV-scale outputs, audit any static local `std::string`, `std::vector`, or `std::unordered_set` used as scratch buffers inside per-function recompilation loops.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp LibertyRecomp/main.cpp LibertyRecompLib/rexglue_runtime_stubs.cpp glue/rexglue-sdk-main/include/rex/ppc/memory.h` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 4 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remained: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, Microsoft-goto warnings, existing `ctx.lr` printf format warning, and existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-vtbl-stack-78795e19-2075-4b33-9468-98553aa817f0.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-vtbl-stack-78795e19-2075-4b33-9468-98553aa817f0.log`
- Smoke result:
  killed intentionally at the bounded smoke timeout. Counts included `MISSING_FUNC_COUNT=11`, and the watchpoint produced the caller stack evidence above.
- Temporary `portable.txt` and the temporary executable-root `game` junction were removed after the smoke run.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `LibertyRecomp/main.cpp`,
  `LibertyRecompLib/rexglue_runtime_stubs.cpp`,
  `glue/rexglue-sdk-main/include/rex/ppc/memory.h`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `docs/dev/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Apply the minimal root-cause fix in `sub_822C1A30`.
   Completion standard: preserve synthetic `stream.ini` config writes, but do not clear `0x82000000-0x82020000`.
2. Rebuild and run bounded smoke.
   Completion standard: `MISSING_FUNC_COUNT` from the zeroed-vtable cluster drops or changes to a later blocker, and no precise exception keywords appear.
3. Remove or lower the temporary page-watch/store-watch diagnostics after the fix is proven.
   Completion standard: final committed runtime code does not carry expensive global store logging unless a follow-up investigation still needs it.
4. Rebuild, bounded smoke, update this guide, commit, push, and continue.
   Completion standard: fresh verification is recorded and the branch is pushed to `origin/codex/switch-audit-20260615`.

Next stage entry condition:

- Start with the `sub_822C1A30` clear removal, then verify whether the current zero-vtable `MISSING-FUNC` cluster is gone before mapping any new blocker.

## 2026-06-18 Windows Continuation 68: VTable Clear Fixed And Probe Cleanup

Current mainline goal:

- Continue Windows runtime bring-up first, using ReXGlue generated `__imp__` implementations as the source of truth and local wrappers only for audited platform glue.
- Keep Switch frozen at LibertyRecompExeFs / NSP-like pre-guest baseline unless explicitly testing Switch.
- Do not treat the prior zero-vtable `MISSING-FUNC` cluster as wrapper debt anymore; it was caused by a synthetic memory clear.

Completed in this batch:

- Applied the root-cause fix in `LibertyRecomp/kernel/imports.cpp`.
  - `sub_822C1A30` still writes the synthetic `stream.ini` config values.
  - It no longer clears `0x82000000-0x82020000`.
  - The range is preserved because it belongs to the loaded GTA IV XEX image and contains live data/vtables.
- Removed temporary heavy diagnostics after proving the fix:
  - ReXGlue `PPC_STORE_*` / `PPC_MM_STORE_*` watched-store logging was removed.
  - Liberty runtime `rexcrt_memcpy`, `rexcrt_memset`, and `rexcrt_XMemCpy` watched-native-write logging was removed.
  - Windows page-protection vtable watchpoint and loader vtable dumps were removed.
- Final scoped code diff is only the `sub_822C1A30` clear removal.

Fresh verification:

- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp LibertyRecomp/main.cpp LibertyRecompLib/rexglue_runtime_stubs.cpp glue/rexglue-sdk-main/include/rex/ppc/memory.h` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 4 LibertyRecomp`
- Build result:
  the long compile completed in the background after the first tool timeout; a fresh follow-up invocation returned `ninja: no work to do`.
- Clean bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-preserve-vtbl-clean-3883b6ba-68c5-48bf-833e-146d4092516e.log`
- Clean bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-preserve-vtbl-clean-3883b6ba-68c5-48bf-833e-146d4092516e.log`
- Smoke result:
  killed intentionally at the 45 second bounded smoke timeout.
  Counts: `MISSING-FUNC=0`, `MISSING_FUNC=0`, `VTBL-TRACE=0`, `VTBL-WATCH=0`, `VTBL-STORE=0`, `VTBL-NATIVE-WRITE=0`, `first-zero=0`, `ASSERT_CB=0`, `ASSERTCB=0`, `TRAP_CONTEXT=0`, `Access violation=0`, `C0000005=0`, `C00000FD=0`, `Unhandled exception=0`, `Exception 0x=0`.
  The run reached `sub_8218BE28 #2500`, `sub_82125478 #1 ENTER`, and `sub_82125478 #1 EXIT`.
  Current visible next blocker class: graphics/resource bring-up, with `volkInitialize failed with error code 0xFFFFFFFD` and `Graphics interface creation returned null` appearing once.
- Temporary `portable.txt` and the temporary executable-root `game` junction were removed after smoke.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`,
  `.planning/`,
  `docs/dev/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push the vtable-clear root-cause fix.
   Completion standard: only `imports.cpp` and this guide are staged, commit message states the audited boundary, and branch `codex/switch-audit-20260615` is pushed.
2. Classify the graphics interface creation failure.
   Completion standard: identify whether `Graphics interface creation returned null` is host graphics configuration, backend selection, or a missing ReXGlue/Xenia GPU bridge side effect.
3. Trace the resource/VFS misses now visible past the old vtable blocker.
   Completion standard: explain why `platform:/rain.dds` and `platform:/rainanim.dds` return 0, and determine whether they should map through the real content root or stay optional.
4. Run another bounded smoke after the next minimal fix.
   Completion standard: no precise exception keywords, no reintroduced `MISSING_FUNC`, and the next blocker is classified with source/register evidence.

Next stage entry condition:

- Resume from graphics/resource bring-up after the vtable clear fix is committed and pushed.
- Do not reintroduce global ReXGlue memory-store instrumentation unless a new root-cause investigation specifically requires it.

## 2026-06-18 Windows Continuation 69: Graphics Null Classified As Vulkan Fallback

Current mainline goal:

- Continue Windows runtime bring-up after the vtable preservation fix.
- Keep ReXGlue/Plume backend behavior as source evidence before editing GPU code.
- Avoid treating host backend fallback logs as guest runtime blockers unless `Video::CreateHostDevice` actually fails.

Completed in this batch:

- Classified the smoke log sequence:
  - `Trying graphics backend: D3D12.`
  - `Trying graphics backend: Vulkan.`
  - `volkInitialize failed with error code 0xFFFFFFFD.`
  - `Graphics interface creation returned null.`
  - `Trying graphics backend: D3D12.`
- Source evidence from `LibertyRecomp/gpu/video.cpp`:
  - `Video::CreateHostDevice` first tries D3D12 on Windows when `LIBERTY_RECOMP_D3D12` is enabled.
  - In Auto mode, after a D3D12 device is created, the code may redirect AMD old-driver or Intel devices to Vulkan.
  - If Vulkan interface creation returns null, the loop can retry D3D12.
  - The function returns false only if `g_device == nullptr`.
- Runtime evidence from the clean smoke:
  - stdout contains `[Main] Video device created`.
  - stderr does not contain `Graphics device creation returned null`.
  - The run continues into guest initialization and reaches `sub_82125478 #1 EXIT`.
- Build layout evidence:
  - D3D12 support is enabled (`LIBERTY_RECOMP_D3D12=ON`).
  - D3D12 runtime DLLs are present beside the executable:
    `dxcompiler.dll`, `dxil.dll`, `D3D12/D3D12Core.dll`, and `D3D12/d3d12SDKLayers.dll`.

Conclusion:

- `Graphics interface creation returned null` in the current smoke is not the active runtime blocker.
- It is a failed Vulkan fallback attempt after D3D12 was viable, most likely because the host Vulkan loader/runtime is unavailable in this environment.
- Do not change GPU backend selection for this message alone. The next real boundary is resource/VFS behavior visible after video creation.

Fresh verification:

- Source inspection:
  `LibertyRecomp/gpu/video.cpp:1990-2127` and `LibertyRecomp/main.cpp:2310-2318`.
- Log inspection:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-preserve-vtbl-clean-3883b6ba-68c5-48bf-833e-146d4092516e.log`
  and
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-preserve-vtbl-clean-3883b6ba-68c5-48bf-833e-146d4092516e.log`.
- No code change was made for this classification.

Current dirty worktree boundaries:

- Allowed current-stage file:
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`,
  `.planning/`,
  `docs/dev/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this classification note.
   Completion standard: only `CONTINUATION_GUIDE.md` is staged and remote branch reaches the new commit.
2. Trace current resource/VFS misses.
   Completion standard: explain why `platform:/rain.dds` and `platform:/rainanim.dds` return 0 in `sub_827E8180`, using VFS mapping and real content root evidence.
3. Decide whether those two resources are optional, incorrectly mapped, or missing from the content layout.
   Completion standard: if optional, record and move on; if incorrectly mapped, make one minimal VFS fix and smoke it.
4. Run another bounded smoke after the next resource/VFS decision.
   Completion standard: no precise exception keywords, no reintroduced `MISSING_FUNC`, and next blocker is classified by source/log evidence.

Next stage entry condition:

- Start at `sub_827E8180` resource path handling for `platform:/rain.dds` and `platform:/rainanim.dds`.
- Keep graphics backend selection unchanged unless a future smoke actually fails `Video::CreateHostDevice`.

## 2026-06-18 Windows Continuation 70: File/Stream Wrapper ABI Preflight

Current mainline goal:

- Continue Windows runtime bring-up with ReXGlue generated `__imp__` implementations as the first source of truth.
- Keep Switch frozen at the LibertyRecompExeFs / NSP-like pre-guest audit baseline unless explicitly testing Switch.
- Do not enable more VFS file-open behavior until the downstream stream wrapper ABI can safely call generated code without public-alias recursion.

Completed in this batch:

- Investigated the current `sub_827E8180` resource/VFS boundary before changing open behavior.
  - Current `sub_827E8180` extracts and logs paths, bypasses shader file requests, then returns `0` for every other non-empty path.
  - The current smoke shows this for many resources, including `common:/DATA/LOADINGSCREENS_360.DAT`, `platform:/data/TIMECYC.DAT`, `platform:/data/effects/gtaRainEmitter.xml`, `platform:/rain.dds`, and `platform:/rainanim.dds`.
  - `platform:/rain.dds` and `platform:/rainanim.dds` are requested twice each and still return `0`.
- Compared the current code against the local `imports.cpp.old_runtime_backup`.
  - The backup had a fuller `sub_827E8180` path with `memory:` URL support, VFS lookup, host-side synthetic file handles, and guest FileStream construction.
  - The current file does not have the backup's `s_pcFileHandleTable` / `RegisterPcHandle()` path.
  - `CreateKernelObject<T>()` still allocates via `g_userHeap.AllocPhysical<T>()`; the backup explicitly avoided that for `NtFileHandle` because `std::fstream` must remain in host memory.
  - Therefore directly copying the old VFS-open path or using `CreateKernelObject<NtFileHandle>()` for `sub_827E8180` would be the wrong next fix.
- Checked the user's real extracted GTA IV directory:
  - `default.xex`, `common.rpf`, `xbox360.rpf`, `audio.rpf`, `common/`, and `xbox360/` are present.
  - The extracted tree has no `rain.dds`, `rainanim.dds`, `gtaRainEmitter.xml`, `gtaRainRender.xml`, `fx_Rain`, `visualeffects`, `materials.dat`, `fragment.xml`, `LOADINGSCREENS_360.DAT`, `engineSettings.xml`, `curves.dat`, or `stream.ini` matches.
  - The smoke also reports `xbox360/textures` missing and `aes_key.bin` missing while source RPF archives are present.
- Found six downstream wrapper self-call hazards that must be fixed before enabling more file/stream work:
  - `sub_827E8880`
  - `sub_8285B680`
  - `sub_827E8420`
  - `sub_822F3110`
  - `sub_822F87E0`
  - `sub_822F57A8`
- Verified generated ReXGlue implementations exist for all six wrappers.
- Added a static RED/GREEN regression check for those wrappers:
  - Before editing it failed because each wrapper called the public alias and did not call its generated `__imp__` implementation.
  - After editing it passed.
- Minimal code fix:
  - Added the corresponding `extern "C" void __imp__sub_x(...)` declarations.
  - Changed only each wrapper's internal call-through from `sub_x(ctx, base)` to `__imp__sub_x(ctx, base)`.
  - Did not enable `sub_827E8180` VFS open behavior in this batch.

Fresh verification:

- Static RED result before the fix:
  each of the six wrappers failed with `calls public alias` and `does not call generated __imp__`.
- Static GREEN result after the fix:
  `PASS: wrappers call generated __imp__ implementations`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 4 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remained: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, existing `ctx.lr` printf format warning, and existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-wrapper-vfs-40a50e6a-4c19-4a84-b785-e3058519b241.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-wrapper-vfs-40a50e6a-4c19-4a84-b785-e3058519b241.log`
- Smoke result:
  killed intentionally at the 45 second bounded smoke timeout.
  Counts: `MISSING-FUNC=0`, `MISSING_FUNC=0`, `Access violation=0`, `C0000005=0`, `C00000FD=0`, `Unhandled exception=0`, `Exception 0x=0`.
  The six fixed wrappers were not hit in this smoke (`sub_827E8880=0`, `sub_8285B680=0`, `sub_827E8420=0`, `sub_822F3110=0`, `sub_822F87E0=0`, `sub_822F57A8=0`), so this batch removes latent recursion debt ahead of the file/stream boundary rather than claiming a runtime path advance.
  `sub_827E8180` still logged `350` lines, with `128` non-shader paths returning `0 (not via storage device path)`.
  The run reached `[Main] Video device created`, `sub_8218BE28 #2500`, and `sub_82125478 #1 EXIT`.
  The visible runtime blocker remains file/resource/content behavior: `assertcb sub_82994840` and `tw/td trap hit` begin immediately after `sub_827E8180 #108 path='platform:/data/TIMECYC.DAT' -> returning 0`.
- Temporary `portable.txt` and the temporary executable-root `game` junction were removed after smoke.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_concurrentqueue.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_implot.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/thirdparty_plume.patch`,
  `docs/backups/github-backup-20260615-022231/submodule-diffs/tools_XenonRecomp.patch`,
  `.planning/`,
  `docs/dev/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this file/stream wrapper ABI preflight.
   Completion standard: only `imports.cpp` and this guide are staged, commit message states the audited boundary, and branch `codex/switch-audit-20260615` is pushed.
2. Prepare a host-side file-stream handle boundary before enabling `sub_827E8180` VFS open.
   Completion standard: decide whether to restore a narrow `s_pcFileHandleTable`/synthetic-handle path, reuse/repair existing NT file handles, or introduce a smaller host-only stream table; do not put `std::fstream` in `g_userHeap.AllocPhysical`.
3. Add a failing static/runtime check for the chosen stream handle policy.
   Completion standard: the check proves `sub_827E8180` cannot construct an unsafe guest-heap `std::fstream` handle and that FileStream read/close paths resolve the same synthetic handle.
4. Implement the minimum safe `sub_827E8180` open path for existing regular files only.
   Completion standard: shader bypass remains unchanged, missing files still return `0`, and existing files create a stream that read/close hooks can consume.
5. Run build and bounded smoke.
   Completion standard: no precise exception keywords, no reintroduced `MISSING_FUNC`, and resource/VFS behavior advances from unconditional `returning 0` to either `VFS FOUND` for present files or a classified missing-content/RPF-key blocker.

Next stage entry condition:

- Start with the host-side FileStream handle policy for `sub_827E8180`.
- Do not enable archive/RPF fallback or `memory:` URL handling in the same edit unless a failing test and source evidence prove it is required for the first file-open boundary.

## 2026-06-18 Windows Continuation 71: Host FileStream Handle Boundary

Current mainline goal:

- Continue Windows runtime bring-up first, using ReXGlue generated `__imp__` implementations as the source of truth and project wrappers only for audited platform glue.
- Keep Switch paused at the LibertyRecompExeFs / NSP-like pre-guest baseline unless explicitly testing Switch.
- Establish a safe host-side file-stream boundary before adding any archive/RPF fallback or entering new guest/runtime code.

Completed in this batch:

- Added a host-side synthetic PC file handle table in `LibertyRecomp/kernel/imports.cpp`.
  - `NtFileHandle` owns `std::fstream`, so PC handles are allocated in native host memory with `new (std::nothrow) NtFileHandle()`.
  - The guest `FileStream` object remains a small guest-memory struct allocated from `g_userHeap.AllocPhysical()`.
  - The guest struct stores `PC_STORAGE_DEVICE_ADDR`, a synthetic handle starting at `0x50000001`, the stream position, capacity, and file size.
- Added shared handle resolution/cleanup helpers:
  - `RegisterPcHandle()`
  - `GetPcHandle()`
  - `ClosePcHandle()`
  - `ResolveNativeFileHandle()`
- Updated the PC stream read/seek/size/close wrappers so PC synthetic handles are resolved on the host side and non-PC streams call generated `__imp__` implementations instead of public aliases.
- Tightened `sub_827E8180` so it only opens concrete regular files already exposed by VFS.
  - Shader bypass behavior remains unchanged.
  - Missing files still return `0`.
  - VFS hits that resolve only to a directory now log `VFS RESOLVED NON-FILE` and return `0` instead of attempting to open a directory as a file.
- Did not add RPF/archive fallback, `memory:` URL handling, or broad VFS mapping changes in this batch.

Root-cause / evidence:

- The old backup implementation had a useful `s_pcFileHandleTable` pattern, but using `CreateKernelObject<NtFileHandle>()` would be unsafe here because it allocates in guest/physical memory while `std::fstream` must stay native.
- The current smoke shows many resource paths still missing from the extracted directory layout. `platform:/rain.dds` and `platform:/moon.dds` currently resolve through a broad platform mapping to the `game/xbox360` directory, not to a regular file.
- A recursive search of the user's real extracted GTA IV tree did not find `TIMECYC.DAT`, `rain.dds`, or `rainanim.dds` as loose extracted files. This points the next boundary toward RPF archive/content lookup rather than direct loose-file VFS open.
- The user-provided OZORDI/XenonRecomp commit `207253d67cdef67235805d595999fa0a2e4fcbd9` remains recorded as a future codegen memory-pressure reference: do not use static local scratch containers inside per-function recompilation loops for GTA IV-scale codegen.

Fresh verification:

- Static policy check:
  `PASS: safe host FileStream policy present`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 4 LibertyRecomp`
- Build result:
  succeeded. Existing warnings remained: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, two Microsoft-goto warnings, existing `ctx.lr` printf format warning, and existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- Bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-pcfilestream-regular-e0b61a98-d470-4b78-8af9-975ef5ec7f03.log`
- Bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-pcfilestream-regular-e0b61a98-d470-4b78-8af9-975ef5ec7f03.log`
- Smoke result:
  killed intentionally at the 45 second bounded smoke timeout.
  Counts: `MISSING-FUNC=0`, `MISSING_FUNC=0`, `Access violation=0`, `C0000005=0`, `C00000FD=0`, `Unhandled exception=0`, `Exception 0x=0`, `FileStream=0`, `VFS RESOLVED NON-FILE=3`, `NOT FOUND via VFS=125`, `assertcb=33`, `tw/td trap hit=99`.
  The run reached `[Main] Video device created`, `sub_8218BE28 #2500`, and `sub_82125478 #1 EXIT`.
  Current visible blocker remains resource/content lookup: direct loose-file VFS still cannot satisfy key paths such as `platform:/data/TIMECYC.DAT`, `platform:/rain.dds`, and `platform:/rainanim.dds`.
- Temporary `portable.txt` and temporary executable-root `game` junction were removed after smoke. The earlier PowerShell `Remove-Item` junction cleanup bug was corrected by deleting only the verified junction path with `[System.IO.Directory]::Delete()`.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this host FileStream handle boundary.
   Completion standard: only `imports.cpp` and this guide are staged, commit message states the audited boundary, and branch `codex/switch-audit-20260615` is pushed.
2. Classify the direct VFS miss set.
   Completion standard: list the first blocking loose-file requests, identify which are absent from the extracted tree, and separate optional shader/texture fallbacks from required data files.
3. Inspect current RPF/archive support before editing.
   Completion standard: determine whether existing `NtFileHandle`/RPF helpers can answer `common.rpf`, `xbox360.rpf`, or `audio.rpf` requests for `TIMECYC.DAT`/rain resources without putting host objects in guest memory.
4. Add one failing static or smoke-visible check for the chosen archive/content path.
   Completion standard: the check proves a concrete requested file either resolves to a regular loose file, a specific RPF entry, or remains a documented missing-content blocker.
5. Implement the minimum safe resource/content lookup fix.
   Completion standard: no broad path remapping, no guest-heap `std::fstream`, no public-alias recursion, and smoke advances from directory/non-file or `NOT FOUND via VFS` to a concrete classified next blocker.

Next stage entry condition:

- Start with the VFS/RPF content boundary for `platform:/data/TIMECYC.DAT`, `platform:/rain.dds`, `platform:/rainanim.dds`, and nearby first-failing resource paths.
- Do not enter gameplay or claim playability. This remains Windows runtime scaffolding toward a future Switch-portable baseline.

## 2026-06-18 Windows Continuation 72: VFS/RPF Content Boundary Classification

Current mainline goal:

- Keep Windows runtime bring-up moving through content/resource blockers without broad architecture churn.
- Treat Switch as frozen at the pre-guest audit baseline unless explicitly testing Switch.
- Do not add runtime RPF extraction until the archive/key/tooling evidence is sufficient.

Completed in this batch:

- Classified the direct VFS miss set from the latest bounded smoke.
  - First loose-file misses include `common:/DATA/LOADINGSCREENS_360.DAT`, `audio:/config/engineSettings.xml`, `platform:/config/curves.dat`, `common:/DATA/VISUALSETTINGS.DAT`, `platform:/stream.ini`, `common:/data/materials/materials.dat`, `common:/data/fragments/fragment.xml`, many `platform:/data/decision/*` files, and the visible blocker cluster around `platform:/data/TIMECYC.DAT`.
  - `platform:/rain.dds`, `platform:/moon.dds`, and similar root platform texture requests currently hit a broad VFS platform mapping and resolve to the `game/xbox360` directory, then the new regular-file guard classifies them as `VFS RESOLVED NON-FILE`.
- Checked the real game content tree:
  - Top-level files present: `default.xex`, `common.rpf`, `xbox360.rpf`, `audio.rpf`.
  - Extracted directories present: `common/`, `xbox360/`, and `xbox360/audio/`.
  - A recursive search did not find loose `timecyc.dat`, `timecycle.dat`, `timecyclemod.dat`, `timecyclemodifiers*.dat`, `rain.dds`, `rainanim.dds`, `moon.dds`, `galaxy.dds`, `visualsettings.dat`, `hud.dat`, `hudcolor.dat`, or `loadingscreens_360.dat`.
- Inspected current RPF-related code:
  - `LibertyRecomp/kernel/io/rpf_loader.cpp` exists but is not currently listed in `LibertyRecomp/CMakeLists.txt`, so it is not part of the Windows executable.
  - Backup files `vfs.cpp.clean` / `vfs.cpp.mod_backup` show an older plan to connect `RpfLoader` into `VFS::Resolve()`, but the active `vfs.cpp` does not include or call it.
  - `imports.cpp` still has older raw RPF stream helpers and a hardcoded `common.rpf` offset table, but the active `sub_827E8180` loose-file path does not use them.
- Verified archive/key state:
  - `common.rpf`: `RPF2`, `tocSize=0x00004800`, `entries=477`, `encrypted=0xFFFFFFFF`.
  - `xbox360.rpf`: `RPF2`, `tocSize=0x00007000`, `entries=987`, `encrypted=0xFFFFFFFF`.
  - `audio.rpf`: `RPF2`, `tocSize=0x00000800`, `entries=12`, `encrypted=0xFFFFFFFF`.
  - Searched `D:\GTA4 NS`, the GitHub repo, and the old Codex workspace for `aes_key.bin`; none was found.
  - `tools/rpf_dump.py` currently rejects the valid little-endian `RPF2` magic value `0x32465052`, so it cannot be used as-is for this audit.

Conclusion:

- The current blocker is not a FileStream ABI crash. It is content availability and lookup.
- Loose extracted content is incomplete for the current request set.
- Runtime RPF extraction is not ready to enable because the existing loader is unlinked, the source RPFs are encrypted, and `aes_key.bin` is absent.
- The next code change should be small and evidence-driven:
  either fix VFS platform-prefix semantics so platform paths no longer resolve to directories or wire a verified extraction/content-prep path after the key/tooling gap is solved. Do not add speculative encrypted RPF runtime reads.

Fresh verification:

- Git status after the previous push showed only known unrelated dirty entries:
  `.planning/`, `thirdparty/concurrentqueue`, `thirdparty/implot`, `thirdparty/plume`, and `tools/XenonRecomp`.
- Source inspection:
  `LibertyRecomp/kernel/vfs.cpp`, `LibertyRecomp/kernel/vfs.cpp.clean`, `LibertyRecomp/kernel/io/rpf_loader.cpp`, `LibertyRecomp/kernel/io/rpf_loader.h`, and `LibertyRecomp/CMakeLists.txt`.
- Content inspection:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`.
- Smoke evidence reused from the immediately preceding verified run:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-pcfilestream-regular-e0b61a98-d470-4b78-8af9-975ef5ec7f03.log`.
- No runtime code changed in this classification batch.

Current dirty worktree boundaries:

- Allowed current-stage file:
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this classification note.
   Completion standard: only `CONTINUATION_GUIDE.md` is staged and pushed.
2. Decide the next smallest code boundary.
   Completion standard: choose between a VFS platform-prefix fix or content-prep/RPF extraction tooling, with the reason written in this guide before editing.
3. If choosing VFS platform-prefix fix, write a static check first.
   Completion standard: `platform:/rain.dds` and `platform:/data/TIMECYC.DAT` must not resolve to the `game/xbox360` directory or to `common/data` through the generic `data/` mapping.
4. If choosing content-prep/RPF extraction, locate or generate the AES key and test extraction outside the runtime first.
   Completion standard: a concrete requested file such as `data/timecycle.dat`, `data/loadingscreens_360.dat`, or a platform texture is extracted to a regular file and can be opened by the existing FileStream boundary.

Next stage entry condition:

- Start by choosing the VFS platform-prefix correction unless an AES key/content extraction path becomes available first.
- Keep runtime RPF extraction out of the app until the encrypted archive path is proven outside the runtime.

## 2026-06-19 Windows/Switch Continuation 73: VFS Platform Prefix Correction

Current mainline goal:

- Keep Windows runtime bring-up moving through content/resource blockers without broad architecture churn.
- Keep Switch paused at the verified LibertyRecompExeFs / NSP-like pre-guest baseline, using Switch only for regression checks or narrowly scoped shared-code blockers.
- Do not enter guest/gameplay code and do not claim Switch playability.

Completed in this batch:

- Fixed active `VFS::Resolve()` path-mapping semantics in `LibertyRecomp/kernel/vfs.cpp`.
  - Replaced substring matching on normalized paths with prefix-only matching.
  - Preserved matching against both the normalized full path and the stripped path.
  - Builds the mapped remainder from the matched candidate instead of always using the stripped path.
  - Moved `platform:` / `platform:/` mappings before generic `data/` and `text/` mappings so `platform:/data/...` stays under `xbox360/...` instead of being captured by `common/data/...`.
  - Removed stale platform-root comments from the subdirectory mapping section.
- Fixed a shared Switch/GCC compile blocker in `LibertyRecomp/kernel/imports.cpp`.
  - `sub_829A7DC8` had `goto done` paths before `table2Begin` / `table2End` local `const` declarations.
  - devkitA64 GCC rejected the function because the jump crossed initialization.
  - The declarations were moved before the earlier `goto done` sites; runtime flow and values are unchanged.
- Confirmed the VFS behavior shift in Windows smoke:
  - `platform:/data/TIMECYC.DAT`, `platform:/moon.dds`, `platform:/rain.dds`, and `platform:/rainanim.dds` now resolve as explicit `NOT FOUND via VFS` loose-content misses.
  - `VFS RESOLVED NON-FILE` dropped from `3` to `0`.
  - The next blocker remains content availability / RPF extraction, not a directory false-positive or FileStream ABI crash.

ReXGlue refs status:

- New local refs were discovered but not fully audited in this batch:
  - `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TheOutFit`
  - `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\bo2-recompiled`
  - `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\skate3recomp`
  - `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TDURE`
- `skate3recomp` received only a first-pass read:
  - README confirms a complete ReXGlue project shape with installer flow, generated-source phase, title-update staging, Vulkan desktop dependency, and a Skate-specific ReXGlue SDK fork.
  - CMake uses generated manifests, `generate-all`, `rex::runtime`, and `rexglue_configure_target(skate3)`.
  - Source structure shows a ReXApp-style application layer, ISO installer, title-update installer, user settings, and targeted guest-side override helpers.
- Do not treat the refs review as complete. The next research stage must read `skate3recomp` more deeply first, then compare TheOutFit, bo2-recompiled, and TDURE before adopting patterns.

Fresh verification:

- Static VFS policy check:
  `PASS: VFS platform prefix policy present`.
- Whitespace check:
  `git -c core.whitespace=cr-at-eol diff --check -- LibertyRecomp/kernel/vfs.cpp LibertyRecomp/kernel/imports.cpp` passed.
- Windows build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-build-x64-clang-nomanifest -j 4 LibertyRecomp`
- Windows build result:
  succeeded. Existing warnings remained: `vfs.h` block-comment warning, `imports.cpp` tautological `uint32_t` comparison, existing `ctx.lr` printf format warning, and existing vcpkg applocal warning about missing `dumpbin` / `objdump`.
- Windows bounded smoke stdout:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-out-vfs-platform-postswitch-3862b210-11d1-479f-98e1-63a09048c930.log`
- Windows bounded smoke stderr:
  `C:\Users\JELLYB~1\AppData\Local\Temp\liberty-smoke-err-vfs-platform-postswitch-3862b210-11d1-479f-98e1-63a09048c930.log`
- Windows smoke result:
  killed intentionally at the 45 second bounded smoke timeout.
  Counts: `MISSING-FUNC=0`, `MISSING_FUNC=0`, `Access violation=0`, `C0000005=0`, `C00000FD=0`, `Unhandled exception=0`, `Exception 0x=0`, `VFS FOUND=0`, `FileStream=0`, `VFS RESOLVED NON-FILE=0`, `NOT FOUND via VFS=128`, `sub_827E8180=350`, `assertcb=33`, `tw/td trap hit=99`.
  The run reached `[Main] Video device created`, `sub_8218BE28 #2500`, and `sub_82125478 #1 EXIT`.
- Switch build command:
  `ninja -C C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\liberty-switch-audit-debug -j 2 LibertyRecompExeFs`
- Switch build result:
  succeeded after the `sub_829A7DC8` GCC-safe declaration move.
- Latest default Switch ExeFS Build ID:
  `e5b897b226d906d0e7c4211e8a51100f3f92ecf7`
- Switch `readelf -dW`:
  dynamic flags were `NOW PIE`; no `TEXTREL` was reported.
- Ryujinx smoke log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-19_01-55-26.log`
- Ryujinx smoke result:
  reached Switch pre-main memory-disabled breadcrumb, `main entered`, skipped RomFS in ExeFS/NSO mode, appended SD log, and stopped at missing `sdmc:/switch/LibertyRecomp/game/default.xex` as expected. This is still not gameplay.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `LibertyRecomp/kernel/vfs.cpp`,
  `LibertyRecomp/kernel/imports.cpp`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this VFS platform-prefix / Switch GCC-safe stage.
   Completion standard: only the two runtime files plus this guide are staged, commit message states the audited boundary, and branch `codex/switch-audit-20260615` is pushed.
2. Pause implementation and complete the refs review before the next runtime change.
   Completion standard: read `skate3recomp` application, installer, CMake/codegen, manifests, overrides, and ReXGlue SDK integration deeply enough to list adoptable patterns and non-applicable patterns.
3. Compare the remaining ReXGlue refs.
   Completion standard: inspect TheOutFit, bo2-recompiled, and TDURE for codegen manifests, generated-source handling, runtime app shape, VFS/content install flow, and import/override patterns.
4. Decide the next runtime boundary after refs review.
   Completion standard: choose either a ReXGlue/ReXApp alignment task, a content-prep/RPF extraction task, or another narrow Windows blocker, with evidence written in this guide before editing.

Next stage entry condition:

- Do not continue implementation until the important ReXGlue refs are reviewed and summarized.
- Keep the next code edit narrow and evidence-driven.
- Keep Switch at the default LibertyRecompExeFs / NSP-like audit package baseline unless the next boundary explicitly needs Switch regression.

## 2026-06-19 Windows/Switch Continuation 74: ReXGlue Reference Review

Current mainline goal:

- Keep Switch paused at the verified LibertyRecompExeFs / NSP-like pre-guest baseline.
- Use the local ReXGlue refs to choose the next Windows runtime direction before more implementation work.
- Do not claim Switch or Windows playability from this research stage.

Completed in this batch:

- Created `docs/switch-audit/REXGLUE_REFERENCE_REVIEW.md`.
- Deepened the `skate3recomp` read beyond the earlier first pass:
  - confirmed the complete ReXGlue app shape around `rex::ReXApp`, `REX_DEFINE_APP`, `rex::runtime`, `rexglue_configure_target()`, generated-source CMake targets, manifest templates, ISO/title-update installers, VFS overlays, DLC/content installation, runtime dispatcher hooks, and deliberate generated-source post-patches.
  - recorded what is adoptable and what is game/fork-specific.
- Reviewed `TheOutFit`:
  - confirmed a generated `rexglue.cmake` / `rexglue_setup_target()` app structure,
  - recorded its manual function/switch-table ledger pattern,
  - recorded its ReXGlue SDK patch classification pattern and watchdog diagnostics.
- Reviewed `bo2-recompiled`:
  - confirmed separate `default` and `default_mp` ReXGlue projects,
  - recorded app lifecycle patterns for update roots, runtime cvars, alternate XEX loading, and centralized XAM/network overrides.
- Reviewed `TDURE`:
  - confirmed it is useful mainly as a minimal ReXGlue template reference.
- Compared those refs against the current LibertyRecomp / GTA IV structure:
  - GTA IV generated sources already exist under `glue/rexglue-sdk-main/gta4-recomp/generated`.
  - The generated config exports `PPCImageConfig`.
  - The current build wraps generated sources in `LibertyRecompLib` and links them into the legacy `LibertyRecomp` executable.
  - `LibertyRecomp/main.cpp` remains a legacy bootstrap with Switch audit gates, not a clean `ReXApp` entrypoint.
  - `LibertyRecomp/app.cpp` remains placeholder-style app glue.

Decision:

- The next implementation direction should not be another broad legacy-shell patch.
- Recommended next code boundary is a separate ReXGlue-aligned Windows prototype target for GTA IV:
  - start from a minimal `Gta4ReXApp : rex::ReXApp`,
  - use existing generated `PPCImageConfig` and generated sources,
  - keep the current legacy executable as a short-term smoke harness,
  - do not move Switch to this path until Windows proves it reaches an equal or better bring-up boundary.

Fresh verification:

- This batch is documentation/research only; no runtime source, build system source, generated code, or thirdparty files were edited.
- Verification before commit should include:
  - `git diff --check -- docs/switch-audit/REXGLUE_REFERENCE_REVIEW.md docs/switch-audit/CONTINUATION_GUIDE.md`
  - `git status --short --branch`
- No Windows/Switch rebuild is required for this docs-only stage; the latest runtime verification remains the Continuation 73 Windows smoke plus Switch ExeFS/Ryujinx baseline.

Current dirty worktree boundaries:

- Allowed current-stage files:
  `docs/switch-audit/REXGLUE_REFERENCE_REVIEW.md`,
  `docs/switch-audit/CONTINUATION_GUIDE.md`.
- Existing unrelated dirty entries remain out of scope:
  `.planning/`,
  `thirdparty/concurrentqueue`,
  `thirdparty/implot`,
  `thirdparty/plume`,
  `tools/XenonRecomp`.

Next small tasks:

1. Commit and push this refs-review documentation stage.
   Completion standard: only `REXGLUE_REFERENCE_REVIEW.md` and `CONTINUATION_GUIDE.md` are staged, commit message states the ReXGlue reference review boundary, and branch `codex/switch-audit-20260615` is pushed.
2. Start the ReXGlue-aligned Windows prototype planning boundary.
   Completion standard: inspect the current ReXGlue SDK helper/API shape and choose the smallest target/file layout without editing runtime behavior.
3. Add the prototype only if the plan can isolate it from the current Switch baseline.
   Completion standard: the existing `LibertyRecomp` target remains buildable, Switch audit options remain untouched, and the prototype is Windows-only until proven.
4. Continue content/RPF work as a separate boundary after app-shell alignment is scoped.
   Completion standard: GTA IV loose/RPF2 content preparation remains explicit and does not get hidden inside unrelated app-shell changes.

Next stage entry condition:

- Begin with a Windows-only ReXGlue app-shell prototype plan, not Switch runtime work.
- Do not edit `LibertyRecomp/kernel/imports.cpp`, `kernel/memory.cpp`, or Switch packaging during the planning step.
- Keep the next code edit narrow and reversible; no broad migration until the sidecar target compiles or fails with a recorded blocker.
