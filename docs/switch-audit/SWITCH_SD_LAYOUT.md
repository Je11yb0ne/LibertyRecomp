# LibertyRecomp Switch SD Layout - Audit Draft

This is the current Nintendo Switch audit layout. It is not a playable-port install guide yet.

## Root

Current root:

```text
sdmc:/switch/LibertyRecomp/
```

Expected audit files and folders:

```text
sdmc:/switch/LibertyRecomp/
  LibertyRecomp.nro            optional placement for the homebrew app
  LibertyRecomp.log            audit log, created by the audit package
  config.toml                  settings file; old duplicate-table files are rewritten by current audit builds
  aes_key.bin                  optional AES key fallback for RPF work
  save/
    GTA4SaveData.bin           future save path
  shader_cache/                future shader cache
  temp/                        temporary installer/cache files
  game/
    default.xex                required by the current content preflight
    common/                    extracted common.rpf contents
    xbox360/                   extracted xbox360.rpf contents
    audio/                     optional extracted audio.rpf contents
                               current real Xbox 360 layout may instead use xbox360/audio/
    extracted/                 fallback extracted file tree used by some paths
  RPF DUMP/                    legacy/fallback extracted-content tree
  dlc/
    TLAD/
      default.xex              DLC validation executable, future
    TBOGT/
      default.xex              DLC validation executable, future
```

## Current Runtime Checks

- The current default Switch audit target is `LibertyRecompExeFs` / `LibertyRecompExefs.nsp`.
  Build and run `LibertyRecompNro` only when testing Homebrew Menu / NRO icon, name, or NRO-specific launch behavior.
- The current full audit package checks for:
  `sdmc:/switch/LibertyRecomp/game/default.xex`
- If it is missing, the package shows `Missing game content` and stops before host startup or guest code.
- If it exists, the default package still stops on `Guest memory disabled` because Switch guest memory is intentionally not enabled in the startup-container build.
- A Switch config-audit build can be enabled with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONFIG_LOAD`; it runs `Config::Load()` and stops before content preflight, host startup, module loading, or guest code.
- A Switch install-check audit build can be enabled with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_INSTALL_CHECK`; it runs `Config::Load()`, calls `Installer::checkGameInstall()`, logs whether `game/default.xex` exists, and stops before host startup, module loading, or guest code.
- A Switch content-layout audit build can be enabled with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONTENT_LAYOUT_CHECK`; it runs `Config::Load()`, calls `Installer::checkGameInstall()`, checks `game/default.xex`, extracted `game/common`, `game/xbox360`, top-level `game/audio` or platform `game/xbox360/audio`, optional source RPF archives, legacy `RPF DUMP`, and optional `dlc`, then stops before host startup, VFS initialization, module loading, or guest code.
- A Switch VFS preflight audit build can be enabled with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_VFS_PREFLIGHT`; it builds the host path cache, initializes XAM roots and the recursive VFS index, resolves representative common/platform/audio paths, and stops before host startup, module loading, or guest code. Current Switch VFS indexing skips nonessential `file_size()` total-byte accounting while preserving path lookup entries.
- A Switch module-load preflight audit build can be enabled with `LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT`; it runs the same real-layout/VFS setup, reads `game/default.xex`, logs XEX2 module metadata, probes XEX decrypt/decompression phases, and stops before full `Image::ParseImage()`, guest-memory writes, and guest code. The latest probe decrypts the XEX payload in place but stops at the basic decompression output allocation under the current 16 MiB libnx heap.

## Notes

- The audit package creates `sdmc:/switch/LibertyRecomp` and `sdmc:/switch/LibertyRecomp/game` when SD is usable.
- `Config::Save()` now groups settings by TOML section. Previous audit builds could generate duplicate `[Input]` tables; the current loader detects that old invalid form and rewrites defaults before parsing.
- `default.xex` alone is not enough for gameplay. The extracted `game/common`, `game/xbox360`, and either `game/audio` or `game/xbox360/audio` trees plus many runtime systems are still unresolved.
- A temporary or dummy `default.xex` can only prove the current file-exists preflight path; it is not valid game content.
- The final RomFS/SD split is not decided. Current RomFS is an empty packaging placeholder.
