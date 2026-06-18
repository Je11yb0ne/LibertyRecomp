# Windows ReXGlue Takeover Plan

## Decision

Do not throw away the current LibertyRecomp tree.

The shorter path is a Windows-only sidecar takeover: keep the current legacy
`LibertyRecomp` executable as the known smoke baseline, then add a separate
ReXGlue-native GTA IV app target that uses the existing GTA IV generated
sources and `PPCImageConfig`. ReXGlue should take over app entry, runtime setup,
guest memory, function dispatch, VFS, kernel exports, logging, and graphics
backend selection in that sidecar target before any Switch migration is
attempted.

This is not a playable claim. The target is a smaller, cleaner bring-up loop for
Windows first.

## Why Not A Full Rewrite

The project already contains valuable work that should not be discarded:

- GTA IV `default.xex` and `default_v8.xex` are present under
  `glue/rexglue-sdk-main/gta4-recomp/assets`.
- ReXGlue-generated GTA IV sources already exist under
  `glue/rexglue-sdk-main/gta4-recomp/generated`.
- The generated config exports `PPCImageConfig` and `PPCFuncMappings`.
- The current Windows smoke baseline reaches video-device creation and guest
  startup traces without the previous access violation / stack overflow class.
- Switch is paused at a verified pre-guest audit package baseline and should not
  be destabilized by a Windows runtime migration.

A sidecar target lets us use ReXGlue more directly without breaking the current
legacy target or Switch audit work.

## What ReXGlue Can Take Over

ReXGlue can own these boundaries in the new Windows target:

- `REX_DEFINE_APP` app entry.
- `rex::ReXApp` / `rex::Runtime` construction.
- Guest memory mapping and `PPCFuncMappings` dispatch setup.
- `LoadXexImage("game:\\default.xex")` and module launch.
- Default VFS mounts for `game:` / `d:` and optional update roots.
- Kernel/XAM/CRT export registration through `rex::kernel`.
- ReXCRT file, string, memory, and heap replacements from the current
  `[rexcrt]` config.
- Logging, cvars, debug overlays, audio/input factories, and graphics backend
  injection.

## What ReXGlue Does Not Solve Automatically

ReXGlue is not a one-button GTA IV port. These remain game-specific work:

- Function-boundary corrections, switch tables, and invalid instruction
  metadata.
- GTA IV-specific hooks and overrides that are not covered by `rex::kernel`.
- Encrypted RPF2 / RPF3 content and the missing `aes_key.bin` issue.
- Exact `platform:/...` content behavior and loose-file extraction policy.
- Graphics correctness beyond the generic Xbox 360 GPU runtime.
- Switch renderer/audio/input/memory constraints.

## Current ReXGlue Reality Check

The local wiki is useful, but this repository has version skew:

- The built CLI is
  `0.8.1.32-dev.gf22cd9d`.
- `rexglue init --help` uses manifest-first options:
  `--project-name`, `--xex-path`, `--game-root`, `--project-root`, and
  `--scan-dll`.
- A fresh `rexglue init` probe generated:
  `generated/rexglue.cmake`, a manifest TOML, `src/main.cpp`, and an
  app header.
- That generated helper expects `rexglue_setup_target(<target>)` and links
  `rex::runtime`.
- The vendored SDK source currently exposes `rex::system` from
  `src/system/CMakeLists.txt`; it notes that former `rexruntime` was merged
  into `rexsystem`, but no `rex::runtime` alias was found in that source file.
- The vendored `rex::ReXApp` header is older/smaller than parts of the wiki and
  the generated template comments. The hooks available in this repo are:
  `OnConfigurePaths`, `OnPreSetup`, `OnPostSetup`, `OnCreateDialogs`, and
  `OnShutdown`.
- Do not depend on unavailable hooks such as `OnLoadXexImage` unless the SDK is
  updated or the sidecar uses direct `rex::Runtime` instead of `rex::ReXApp`.

## Vulkan Direction

Vulkan is the preferred Windows graphics direction for later Switch portability,
but it is not automatic.

On Windows, the vendored SDK defaults to `REXGLUE_USE_D3D12=ON` and
`REXGLUE_USE_VULKAN=OFF`. ReXApp also selects D3D12 before Vulkan when both
compile definitions are present. The sidecar target must therefore make Vulkan
an explicit build/config boundary:

- configure the ReXGlue SDK with `REXGLUE_USE_VULKAN=ON`;
- optionally disable D3D12 for the sidecar while testing;
- if both are enabled, override `OnPreSetup(rex::RuntimeConfig&)` and inject
  `rex::graphics::vulkan::VulkanGraphicsSystem`.

Desktop Vulkan still does not become a Switch renderer by itself. Switch still
needs a real deko3d/NVN-style backend or a deliberate portability layer later.

## Shortest Implementation Path

### Current Phase Status

Phase 1 is now verified as a direct `rex::Runtime` tool-mode sidecar, not yet a
full `rex::ReXApp` migration:

- `LibertyRecompRex` is a Windows-only sidecar target gated by
  `LIBERTY_RECOMP_BUILD_REX_SIDECAR`.
- It links against the local ReXGlue prebuilt static libraries.
- It reaches `Runtime initialized in tool mode (no GPU)` and
  `ReXGlue runtime setup reached tool-mode pre-guest boundary` with exit code
  `0`.
- The legacy `LibertyRecomp` target still builds in the same Windows build
  directory.
- Guest XEX loading, generated-source attachment, `PPCImageConfig`, and
  graphics backend selection remain future phases.

Phase 2 is now verified through the generated metadata / function table
boundary, still before XEX load or guest launch:

- `LibertyRecompRex` now links through `LibertyRecompLib`, so it uses the
  existing GTA IV generated sources and generated include path without copying
  or regenerating tracked generated code.
- The sidecar calls
  `rex::Runtime::Setup(PPCImageConfig.code_base, PPCImageConfig.code_size,
  PPCImageConfig.image_base, PPCImageConfig.image_size,
  PPCImageConfig.func_mappings, ...)`.
- The ReXGlue runtime initializes the GTA IV code/image metadata and registers
  `37151` recompiled functions in tool mode.
- The current prebuilt `rexkernel.lib` behaves like a codegen-only kernel build:
  it does not contain `xam_ui.cpp.obj` or `xboxkrnl_crypt.cpp.obj`, while the
  generated mapping table references eight `__imp__` XAM/XboxKrnl symbols from
  those files.
- `LibertyRecompRex/src/pre_guest_import_bridges.cpp` is a sidecar-only
  temporary bridge for those eight symbols so the generated metadata can link.
  It is not a gameplay/runtime implementation and must be replaced with full
  SDK exports or real GTA IV overrides before guest launch work.
- The legacy `LibertyRecomp` target still builds in the same Windows build
  directory.

Phase 3 has started with a content/XEX preflight only; it still does not call
`LoadXexImage()`:

- After `Runtime::Setup(...)`, the sidecar resolves `game:\default.xex` through
  ReXGlue VFS and checks the host file at the configured assets root.
- The verified GTA IV asset file is `11841536` bytes and has XEX2 magic.
- The sidecar now also parses XEX metadata from a host file buffer before
  ReXGlue loader entry:
  `moduleFlags=0x00000001`, `headerSize=0x3000`, `security=0x90`,
  `optHeaders=15`, `imageSize=0x11F0000`, `load=0x82000000`,
  `imageBase=0x82000000`, `entry=0x829A0860`, `fileFormat enc=1 comp=1`,
  `resources=1`, `importLibs=2`, `imports=484`, `pages=287`.
- The metadata preflight confirms `imageBase` and `imageSize` match
  `PPCImageConfig`.
- The sidecar logs the VFS/host result and then stops at the same tool-mode
  pre-guest boundary.
- Real XEX loading, module materialization, import patching, and launch remain
  future work.

`LoadXexImage()` side-effect audit decision:

- It is not a metadata-only call.
- `Runtime::LoadXexImage()` creates a `UserModule`, calls
  `UserModule::LoadFromFile()`, then calls `KernelState::SetExecutableModule()`.
- `UserModule::LoadFromFile()` resolves the VFS path, maps or reads the full
  XEX, then calls `LoadFromMemory()`.
- `LoadFromMemory()` constructs an `XexModule` and calls `XexModule::Load()`.
- `XexModule::Load()` reads headers/security info and calls `ReadImage()`;
  `ReadImage()` resets/allocates the guest image heap, decrypts/decompresses
  into guest memory, and validates the PE image.
- `LoadXexContinue()` reads PE headers from guest memory, parses imports,
  patches variable imports in guest memory, sets memory protection, copies the
  XEX header to system heap, fills loader data, and runs `OnLoad()`.
- `KernelState::SetExecutableModule()` writes process/loader state and starts
  the kernel dispatch host thread.
- Therefore the next boundary should be a sidecar-only XEX metadata preflight
  using the host file/header buffer before deciding whether to cross into
  `LoadXexImage()` guest-memory side effects.

### Phase 1: API And CMake Compatibility Probe

Goal: prove the current repo can build a tiny ReXGlue-native Windows target
without launching GTA IV guest code.

Tasks:

1. Generate or hand-write a minimal Windows-only sidecar target using the
   observed `rexglue init` template shape.
2. Resolve the `rex::runtime` vs `rex::system` target mismatch without changing
   thirdparty code unless evidence proves an SDK alias patch is required.
3. Link only the sidecar against ReXGlue SDK targets.
4. Keep the existing `LibertyRecomp` target unchanged and buildable.

Completion standard:

- Configure succeeds for the existing Windows build directory or a new isolated
  Windows build directory.
- The sidecar target compiles to an executable.
- The legacy `LibertyRecomp` target still builds.
- No guest module is launched yet.

### Phase 2: Existing Generated Sources Into Sidecar

Goal: attach the existing GTA IV generated source set to the sidecar target.

Tasks:

1. Include
   `glue/rexglue-sdk-main/gta4-recomp/generated/sources.cmake`.
2. Include the generated directory so `gta4_config.h` and `gta4_init.h` resolve.
3. Construct the sidecar app with `PPCImageConfig`.
4. Keep tracked generated sources read-only.

Completion standard:

- Sidecar links with the existing generated GTA IV sources.
- Duplicate symbol / missing `__imp__` failures are classified into:
  ReXGlue SDK exports, GTA IV overrides, or legacy-only project glue.
- No broad generated-source regeneration is performed.

### Phase 3: Runtime Setup And XEX Load

Goal: let ReXGlue own memory, VFS, kernel setup, and XEX image loading.

Tasks:

1. Set `game_data_root` to the GTA IV asset/content directory.
2. Let ReXApp/Runtime run `Setup()`.
3. Confirm `LoadXexImage("game:\\default.xex")` succeeds.
4. If ReXApp's fixed default launch path is too restrictive, switch to a direct
   `rex::Runtime` sidecar only after recording why.

Completion standard:

- Runtime setup reaches post-setup logs.
- `default.xex` loads from the expected `game:` path.
- Failures are logged as missing content/import/config blockers, not silent
  crashes.

### Phase 4: Launch And Minimal Override Migration

Goal: launch guest startup under ReXGlue and migrate only required GTA IV
overrides.

Tasks:

1. Port only the imports/hooks needed by the first blocker.
2. Prefer ReXGlue macros and strong wrapper patterns:
   `PPC_HOOK`, `PPC_STUB`, `XAM_EXPORT`, `XBOXKRNL_EXPORT`, `REXCRT_EXPORT`.
3. Keep each migrated override in a small, subsystem-named file.
4. Compare each run against the legacy smoke baseline.

Completion standard:

- The sidecar reaches an equal or better startup boundary than the current
  legacy smoke.
- New blockers are logged with guest address, host symbol, and content path
  evidence.
- The legacy target remains available as a comparison harness.

### Phase 5: Content/RPF Boundary

Goal: make GTA IV content resolution explicit instead of hiding it inside VFS
patches.

Tasks:

1. Define the Windows game-data layout expected by the sidecar.
2. Decide whether the next step needs loose extracted files, an RPF device, or
   a key/extraction flow.
3. Keep `platform:/...` path handling separate from the app-shell migration.

Completion standard:

- The sidecar has a documented content layout.
- Known missing paths such as `platform:/data/TIMECYC.DAT`,
  `platform:/rain.dds`, and `platform:/rainanim.dds` are assigned to a concrete
  content strategy.

## Do Not Do Yet

- Do not delete or replace the current `LibertyRecomp` executable.
- Do not move Switch to ReXGlue sidecar code.
- Do not regenerate tracked GTA IV generated sources without a temp diff and a
  recorded decision.
- Do not edit `thirdparty` submodules for this planning phase.
- Do not continue broad wrapper churn in `LibertyRecomp/kernel/imports.cpp`
  before the sidecar CMake/API probe is attempted.
- Do not claim Windows or Switch playability.

## Next Small Tasks

1. Create the sidecar target skeleton in an isolated, Windows-only way.
   Completion standard: CMake sees the target without affecting Switch.
2. Resolve `rex::runtime` target availability.
   Completion standard: either the package provides it, or a local sidecar-only
   compatibility link path is documented and verified.
3. Build the sidecar without guest launch.
   Completion standard: executable links, legacy target still builds.
4. Attach existing generated GTA IV sources.
   Completion standard: compile/link blockers are classified, not guessed.
5. Decide Vulkan build mode for the sidecar.
   Completion standard: configure flag and `OnPreSetup` policy are recorded.

## Entry Condition For Switch Return

Switch should stay paused until the Windows sidecar either:

- reaches the same or better startup boundary than the current legacy Windows
  smoke, or
- proves a hard blocker that must be solved in codegen/content/runtime before
  Switch can benefit.
