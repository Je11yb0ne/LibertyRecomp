# LibertyRecomp Switch Continuation Guide

## Current Mainline Goal

Continue the LibertyRecomp / GTA IV Nintendo Switch audit until the Switch startup container, SD content layout, VFS path/index layer, module-loading preflight, and guest-memory/page-backing boundaries are stable, reproducible, and documented.

This is not a playable Switch port. All current Switch artifacts are audit packages.

## Current Stage Goal

Define the next Switch-safe module image memory strategy before attempting full `Image::ParseImage()` or guest-memory writes. The immediate blocker is no longer content layout, VFS, or XEX metadata; it is host-side XEX image expansion under the current Switch heap budget.

Current finding: a first full `Image::ParseImage()` attempt reached `default.xex` read success (`module bytes=11841536`) and did not return within the 240-second Ryujinx window. The subsequent Switch-local phase probe narrowed that broad stall to host-loader memory pressure: duplicate decrypted-buffer allocation can enter the GCC unwinder path, while in-place AES decryption completes and the next `0x11F0000` decompression output allocation fails cleanly.

Previous stage result: lightweight XEX metadata preflight passes in Ryujinx with the real staged layout. It reads `default.xex`, validates the XEX2 header bounds, logs security/file-format/resource/import metadata, and stops before `Image::ParseImage()`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.

Current stage rule: do not modify `tools/XenonRecomp` yet. First reproduce or narrow the parse behavior with local/read-only evidence, then add Switch-local audit instrumentation only if needed.

Local host-side profiling result: a read-only parser against `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)\default.xex` completed XEX key decrypt, image decrypt, basic decompression, and PE section scan in about `461 ms` on Windows. It reported three basic-compression blocks, `13` PE sections, `imageSize=0x11F0000`, `imageBase=0x82000000`, and entry `0x829A0860`. This suggests the 240-second Ryujinx stall is not simply that the source XEX is too large to parse; the next minimal Switch boundary should log equivalent phases inside the Switch audit path before calling full `Image::ParseImage()`.

Latest Switch result: the Switch-local full-parse phase probe proved that in-place AES-CBC decryption of the `11829248` byte XEX payload completes in Ryujinx, but the next basic decompression output allocation needs `18808832` bytes (`0x11F0000`) and fails under the current 16 MiB libnx heap while the loaded XEX buffer is still resident. Earlier duplicate-buffer attempts entered the GCC unwinder path (`Unknown MRS ... gcspr_el0`) during allocation. Continue by designing a Switch-safe host loader memory strategy: in-place decrypt plus staged/streamed decompression, a temporary audit heap increase, or a dedicated module-loader arena. Do not call `Image::ParseImage()` unchanged until this boundary is addressed.

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

1. Baseline default ExeFS verification.
   Completion standard: `LibertyRecompExeFs` builds, `readelf -dW` reports no `TEXTREL`, Ryujinx missing-content smoke reaches `Early preflight missing game executable`, and no VFS/module/guest path is entered.

2. Choose the next module-loader memory boundary.
   Completion standard: document whether the next audit build will test a temporary heap increase, a malloc-backed loader arena, or streaming/staged basic decompression, and state why it remains before guest-memory writes.

3. Implement the smallest loader-memory experiment.
   Completion standard: change only `LibertyRecomp/main.cpp` and, if absolutely needed, one Switch CMake heap/audit option; keep it behind `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT`.

4. Verify the loader-memory experiment in Ryujinx with real staged layout.
   Completion standard: `LibertyRecompExeFs` builds, no `TEXTREL`, Ryujinx log shows whether decompression allocation/streaming succeeds, and the audit still stops before `LdrLoadModule()` guest-memory writes and `GuestThread::Start()`. Temporary SD hardlinks/junctions are removed and PTC is restored.

5. Restore default ExeFS and smoke-test.
   Completion standard: default CMake cache has all Switch audit stops OFF and guest-memory audit OFF; default Ryujinx smoke returns to missing-content behavior without entering module-load preflight.

6. Document, commit, and push.
   Completion standard: update this guide first, then audit log/stub/layout docs as needed; sync updated switch-audit docs to `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns`; commit only scoped files; push current `codex/switch-audit-20260615` branch.

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
- Restored default ExeFS Build ID after full-parse probe:
  `89b13bdec0d3610a5afdb5d626296ffe17427e14`
- Restored default ExeFS Ryujinx smoke log after full-parse probe:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-23-38.log`

## Latest Verified Baselines

- Branch: `codex/switch-audit-20260615`
- Latest pushed commit before this guide: `16d1358e Harden Switch VFS recursive index audit`
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
- Latest restored default ExeFS Build ID:
  `89b13bdec0d3610a5afdb5d626296ffe17427e14`
- Latest restored default ExeFS Ryujinx log:
  `D:\Games\Ryujinx\Ryujinx\portable\Logs\Ryujinx_Canary_1.3.269_2026-06-15_23-23-38.log`
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
- and a documented decision about whether the next boundary is full image parse instrumentation, module import parsing, guest-memory/page backing, or another host-side preflight blocker.

## Next Boundary Decision

The next smallest useful boundary is module-loader memory strategy. Prefer an audit-only path in `LibertyRecomp/main.cpp` that keeps decryption in-place and either streams/stages basic decompression or tests a documented temporary heap/arena increase, then stops before `LdrLoadModule()` writes into guest memory. Only modify `tools/XenonRecomp` after documenting why a submodule-local change is unavoidable.
