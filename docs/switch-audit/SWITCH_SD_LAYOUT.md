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
  LibertyRecomp.log            audit log, created by the NRO
  config.toml                  future settings file
  aes_key.bin                  optional AES key fallback for RPF work
  save/
    GTA4SaveData.bin           future save path
  shader_cache/                future shader cache
  temp/                        temporary installer/cache files
  game/
    default.xex                required by the current content preflight
    common/                    extracted common.rpf contents
    xbox360/                   extracted xbox360.rpf contents
    audio/                     extracted audio.rpf contents
    extracted/                 fallback extracted file tree used by some paths
  RPF DUMP/                    legacy/fallback extracted-content tree
  dlc/
    TLAD/
      default.xex              DLC validation executable, future
    TBOGT/
      default.xex              DLC validation executable, future
```

## Current Runtime Checks

- The current full NRO checks for:
  `sdmc:/switch/LibertyRecomp/game/default.xex`
- If it is missing, the NRO shows `Missing game content` and stops before host startup or guest code.
- If it exists, the current NRO still stops on `Guest memory disabled` because Switch guest memory is intentionally not enabled in the startup-container build.

## Notes

- The NRO creates `sdmc:/switch/LibertyRecomp` and `sdmc:/switch/LibertyRecomp/game` when SD is usable.
- `default.xex` alone is not enough for gameplay. The extracted `game/common`, `game/xbox360`, and `game/audio` trees plus many runtime systems are still unresolved.
- The final RomFS/SD split is not decided. Current RomFS is an empty packaging placeholder.
