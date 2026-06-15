# LibertyRecomp Switch Continuation Guide

## Current Mainline Goal

Continue the LibertyRecomp / GTA IV Nintendo Switch audit until the Switch startup container, SD content layout, VFS path/index layer, module-loading preflight, and guest-memory/page-backing boundaries are stable, reproducible, and documented.

This is not a playable Switch port. All current Switch artifacts are audit packages.

## Current Stage Goal

Prepare the next guest-memory/page-backing audit boundary before attempting real `LdrLoadModule()` image materialization or guest/gameplay execution. The module-load preflight can now reach host-side XEX metadata, in-place decrypt, staged basic-decompression view, PE section scan, and import thunk scan without allocating the full decompressed image.

Current finding: a first full `Image::ParseImage()` attempt reached `default.xex` read success (`module bytes=11841536`) and did not return within the 240-second Ryujinx window. The subsequent Switch-local phase probe narrowed that broad stall to host-loader memory pressure: duplicate decrypted-buffer allocation can enter the GCC unwinder path, while in-place AES decryption completes and the next `0x11F0000` decompression output allocation fails cleanly.

Previous stage result: lightweight XEX metadata preflight passes in Ryujinx with the real staged layout. It reads `default.xex`, validates the XEX2 header bounds, logs security/file-format/resource/import metadata, and stops before `Image::ParseImage()`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.

Current stage rule: do not modify `tools/XenonRecomp` yet. First reproduce or narrow the parse behavior with local/read-only evidence, then add Switch-local audit instrumentation only if needed.

Local host-side profiling result: a read-only parser against `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)\default.xex` completed XEX key decrypt, image decrypt, basic decompression, and PE section scan in about `461 ms` on Windows. It reported three basic-compression blocks, `13` PE sections, `imageSize=0x11F0000`, `imageBase=0x82000000`, and entry `0x829A0860`. This suggests the 240-second Ryujinx stall is not simply that the source XEX is too large to parse; the next minimal Switch boundary should log equivalent phases inside the Switch audit path before calling full `Image::ParseImage()`.

Latest Switch result: the staged basic-decompression view avoids the `0x11F0000` output allocation, reads PE/import slices directly from the decrypted payload, and completes the module preflight in Ryujinx. It finds `13` PE sections, `2` import libraries, `484` import descriptors, and `0` missing thunk targets, then stops before `Image::ParseImage()`, `LdrLoadModule()` guest-memory writes, and `GuestThread::Start()`.

Current boundary decision: test only the guest-memory ranges that `LdrLoadModule()` would need next, without calling `Image::ParseImage()` and without copying the full image. The planned audit range is the XEX image span `0x82000000..0x831F0000` (`imageSize=0x11F0000`). The subordinate ranges are the resource pointer `0x83150000..0x831EBA69`, collision zero `0x82003880..0x82003900`, stream struct `0x82003890..0x820038AC`, and worker globals `0x830F5000..0x830F8000`. These subordinate ranges all fall inside the image span. The probe runs under `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT` with `LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT=ON`, logs the planned ranges, touches only first/last bytes of each mapped range with save/restore semantics, and still stops before real module image copy, XDBF wrapper setup, `GuestThread::Start()`, or generated PPC code.

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

1. Baseline default ExeFS verification.
   Completion standard: `LibertyRecompExeFs` builds, `readelf -dW` reports no `TEXTREL`, Ryujinx missing-content smoke reaches `Early preflight missing game executable`, and no VFS/module/guest path is entered.

2. Define the next guest-memory/page-backing boundary.
   Completion standard: document the exact range(s) that would be touched by `LdrLoadModule()` image copy/resource setup and use the existing sparse guest-memory audit backend with a narrower module-image translate/touch probe.

3. Implement the smallest pre-guest memory probe.
   Completion standard: keep the probe behind `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT`, do not start `GuestThread`, and do not run generated PPC code. Guest-memory writes are limited to first/last-byte save/restore touches for the logged module image/resource/collision/stream/worker ranges.

4. Verify the pre-guest memory probe in Ryujinx with real staged layout.
   Completion standard: `LibertyRecompExeFs` builds, no `TEXTREL`, Ryujinx log shows mapped range decisions and the audit stops before `GuestThread::Start()`. Temporary SD hardlinks/junctions are removed and PTC is restored.

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

## Latest Verified Baselines

- Branch: `codex/switch-audit-20260615`
- Latest pushed commit before this guide: `c367be24 Audit Switch staged XEX import preflight`
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
- and a documented result for the module image guest-memory translate/touch probe, including whether `0x82000000..0x831F0000` is backed under the current sparse audit mapping.

## Next Boundary Decision

The module image guest-memory translate/touch boundary is now verified under the narrow sparse-memory profile. The next smallest useful boundary is to materialize the staged XEX image directly into the mapped guest image span under the module-load audit stop, then verify the same resource/collision/stream/worker ranges without calling `GuestThread::Start()` or generated PPC code. Do not use the existing full `Image::ParseImage()` path for this yet; it still implies avoidable heap pressure from full host-side image allocation.
