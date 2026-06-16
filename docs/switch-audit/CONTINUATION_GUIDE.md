# LibertyRecomp Switch Continuation Guide

## Current Mainline Goal

Mainline work is now Windows runtime bring-up first, with ReXGlue as the primary runtime/codegen reference and XenonRecomp as an auxiliary translation/reference aid. The current Windows objective is to move the legacy LibertyRecomp bootstrap through guest startup blockers by fixing wrapper recursion, MMIO handling, VBlank/thread startup, module initialization, guest memory/page backing, content paths, and startup logging one verified boundary at a time.

Switch work is paused at the verified LibertyRecompExeFs / NSP-like pre-guest audit baseline. Use Switch only for regression checks, packaging/content-layout fixes, or deliberately scoped pre-guest blockers while Windows runtime work is the mainline. This is not a playable Switch port; all current Switch artifacts are audit packages.

## Tooling Priority

Use ReXGlue SDK as the primary runtime/codegen reference for ongoing LibertyRecomp / GTA IV work. The current project is already built around ReXGlue generated sources under `glue/rexglue-sdk-main/gta4-recomp/generated`, and wrapper/debugging work should treat generated `__imp__sub_x` implementations as the source of truth when a project-side strong wrapper hooks `sub_x`.

Use XenonRecomp as an upstream translation/reference aid, not the main runtime integration target. Do not modify `tools/XenonRecomp` unless a stage explicitly proves a translation-side change is required and records why first.

Local read-only references:

- ReXGlue SDK: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\rexglue-sdk`
- Recompiled GTA IV samples: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\Recompiled-Samples`
- XenonRunner runtime sample: `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\XenonRunner`

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

1. Inspect the Windows stack overflow now landing in `sub_82197338 + 0x202`.
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
