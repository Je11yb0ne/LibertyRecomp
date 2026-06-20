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

Phase 3 has advanced from content/XEX preflight to a controlled
`LoadXexImage()` audit gate. The default sidecar path still does not call
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
- Passing `--audit-load-xex` explicitly crosses the ReXGlue module materialize
  boundary without calling `LaunchModule()`.
- The opt-in audit currently loads `game:\default.xex`, materializes the guest
  image, parses import libraries, patches variable imports, creates symbols for
  `xam` with `87` imports and `xboxkrnl` with `160` imports, and returns
  status `00000000`.
- The opt-in audit logs the existing missing optional patch file warning for
  `default.xexp` and the existing unimplemented variable import warning for
  `xboxkrnl:0x1b (ExThreadObjectType)`.
- `LoadXexImage()` still starts ReXGlue's kernel dispatch host thread through
  `KernelState::SetExecutableModule()`. It is not a no-side-effect preflight.
- The opt-in audit now logs public post-load module state from ReXGlue:
  `name=default`, `path=\Device\Harddisk0\Partition1\default.xex`,
  `title=0x545407F2`, `entry=0x829A0860`, `stack=0x00040000`,
  `hmodule=0x0001B000`, `guest_xex_header=0x0001C000`,
  `image_base=0x82000000`, `image_size=0x011F0000`, `pages=287`,
  `sections=13`, `executable_sections=2`, `writable_sections=6`,
  `import_libs=2`, and loaded import entries `247`.
- The opt-in audit also logs import/export coverage for the temporary bridge
  symbols:
  - XAM UI functions at ordinals `0x02C6`, `0x02CB`, `0x02D5`, `0x02D9`,
    and `0x02DC` have resolver table entries but are not resolver-implemented
    and are not registered in the ReXGlue PPC function registry.
  - XboxKrnl crypto/key functions at ordinals `0x0192`, `0x0256`, and
    `0x0257` have resolver table entries but are not resolver-implemented and
    are not registered in the ReXGlue PPC function registry.
  - `ExThreadObjectType` at `xboxkrnl.exe:0x001B` is a variable export with a
    resolver table entry but no variable mapping.
- Guest main launch remains disabled because `LaunchModule()` is not called.
- This is a module materialization audit only, not a playable state.

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
- That side-effect classification led to the now-verified sidecar-only XEX
  metadata preflight using the host file/header buffer.
- The current implementation only crosses this boundary when the user passes
  `--audit-load-xex`. The normal smoke path continues to prove the safe
  metadata/pre-guest boundary first.
- The post-`LoadXexImage()` module-state diagnostics are now verified while
  still keeping `LaunchModule()` disabled.
- The import/export coverage diagnostics are now verified. The next boundary is
  replacing the temporary sidecar import bridge, most likely by building/linking
  a non-codegen-only ReXGlue kernel or adding real sidecar exports with recorded
  rationale.
- A non-codegen-only ReXGlue kernel was tested in an isolated temp SDK/build
  root. The D3D12 path is blocked by missing `thirdparty/dxbc`, while a
  Vulkan-only configure succeeds with `REXGLUE_CODEGEN_ONLY=OFF`,
  `REXGLUE_USE_D3D12=OFF`, `REXGLUE_USE_VULKAN=ON`,
  `CMAKE_POLICY_VERSION_MINIMUM=3.5`, and `CMAKE_CXX_FLAGS=/GR /EHsc`.
  The Vulkan build reaches `rexaudio` and then fails because the repository SDK
  snapshot does not define real FFmpeg `libavcodec` / `libavutil` CMake targets
  that propagate `thirdparty/FFmpeg` includes. The full-kernel path is therefore
  SDK packaging/CMake integration debt, not a verified drop-in bridge
  replacement yet.
- Reference comparison now favors a narrow sidecar compatibility step before a
  full SDK import. `skate3recomp` uses the cleaner long-term shape:
  `add_subdirectory` for a full ReXGlue SDK source tree, `rex::runtime`,
  `REX_DEFINE_APP`, generated sources, and ReXApp hooks for path/content
  behavior. `bo2-recompiled`, `TDURE`, and `TheOutFit` use the generated
  `rexglue_setup_target(...)` pattern. `bo2-recompiled` also shows
  project-side stubs are a normal short-term compatibility layer, but those are
  still explicit project code, not proof of full kernel coverage.
- The sidecar bridge has therefore been narrowed from unregistered linker-only
  `PPC_STUB_LOG(...)` definitions to ReXGlue-registered
  `XAM_EXPORT_STUB(...)` / `XBOXKRNL_EXPORT_STUB(...)` definitions. The opt-in
  `--audit-load-xex` path now logs `ppc_registered=yes` with
  `bridge=sidecar_registered_stub` for all eight bridge functions while still
  leaving `LaunchModule()` disabled.
- `ExThreadObjectType` is now covered by a sidecar-only variable mapping. After
  `Runtime::Setup(...)` and before any XEX load, the sidecar allocates a
  minimal `rex::system::X_OBJECT_TYPE`, sets `pool_tag=Thrd`, and calls the
  public `ExportResolver::SetVariableMapping("xboxkrnl.exe", 0x001B, ...)`.
  The opt-in `--audit-load-xex` path now logs `resolver_implemented=yes`,
  a nonzero variable pointer, `bridge=sidecar_variable_mapping`, and no longer
  reports `Unimplemented variable import: xboxkrnl.exe:0x1b`. This is still a
  sidecar compatibility shim; the fuller SDK reference owns this properly via
  `KernelGuestGlobals` in `KernelState` and `XboxkrnlModule`.
- A disabled first-launch gate now exists as `--audit-launch-module`. The flag
  implies `--audit-load-xex`, logs `Audit LaunchModule: requested-disabled`,
  records a pre-launch module summary, and names the next proof target as
  `host-thread-create-or-first-guest-pc`, but it deliberately does not call
  `KernelState::LaunchModule()`. Default and `--audit-load-xex` runs remain
  non-launching.
- The disabled first-launch gate now also logs a structured capture plan:
  `process_exit_code=caller`, `structured_exception=planned`,
  `last_log_line=LibertyRecompRex.log`, `guest_entry_pc=0x829A0860`,
  `host_thread_create=planned`, and `first_import_call=planned`. These are
  planned capture fields only; real structured exception / host-thread /
  first-import capture mechanics still need to be implemented before enabling
  `LaunchModule()`.
- A first-launch failure-capture observer is now installed only for
  `--audit-launch-module`. It uses ReXGlue's
  `rex::arch::ExceptionHandler` chain after MMIO setup, logs unhandled
  access-violation / illegal-instruction observations, flushes all ReXGlue
  loggers, and returns `false` so it continues search rather than swallowing
  the exception. Default and `--audit-load-xex` runs do not install it.
- Current ReXGlue source differs from the fuller ReXApp lifecycle described in
  the wiki: this repository's `KernelState::LaunchModule()` resumes the main
  `XThread` before returning. Any direct-runtime first-launch attempt must
  therefore log returned thread metadata immediately and treat it as
  post-resume evidence, not a suspended pre-resume hook.
- The first real `Runtime::LaunchModule()` attempt is now enabled only behind
  `--audit-launch-module`. It records returned `XThread` metadata and PPC
  context state, uses a measured bounded observation window, flushes logs, and
  exits with audit code `8` if the main guest thread is still running after the
  observation window. Default and `--audit-load-xex` remain non-launching.
- With the sidecar's current default assets root, first launch reaches
  `XThread::Execute - Calling function at 829A0860`, then fails the first
  observed content lookup for `game:\common.rpf` (`0xc000000f`) because that
  assets directory only contains `default.xex` and `default_v8.xex`. The
  failure-capture observer also logs an access violation at host fault
  `0x0000000100000000` with access `write`, and the bounded audit exits with
  code `8`.
- A fuller local GTA IV root is available for the next run:
  `D:\GTA4 NS\Grand Theft Auto IV (USA) (En,Fr,De,Es,It)`, containing
  `default.xex`, `common.rpf`, and `xbox360.rpf`.
- Full-root gated launch confirms the content-root blocker clears:
  `game:\common.rpf`, `game:\xbox360.rpf`, and `game:\audio.rpf` all resolve
  to host files. The new first runtime/API blocker is
  `NtQueryInformationFile(XFileSectorInformation) unimplemented` in
  `glue/rexglue-sdk-main/src/kernel/xboxkrnl/xboxkrnl_io_info.cpp`, followed
  by the failure-capture observer logging an access violation at fault
  `0x0000000000000000` with access `read`.
- `XFileSectorInformation` is now covered for the Windows sidecar by compiling
  `glue/rexglue-sdk-main/src/kernel/xboxkrnl/xboxkrnl_io_info.cpp` directly
  into `LibertyRecompRex` and porting the newer ReXGlue 4-byte path-hash stub
  response. This overlays the stale prebuilt `rexkernel.lib` object for this
  one file-info implementation without changing generated GTA IV sources or
  ReXGlue binary libraries. Full-root `--audit-launch-module` now logs
  `Stub XFileSectorInformation!` twice and no longer logs the sector-info
  unimplemented blocker; the next observed blocker is a first-launch guest
  thread access violation at fault `0x0000000000000000` with access `read`.

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
3. Confirm `LoadXexImage("game:\\default.xex")` succeeds behind the explicit
   `--audit-load-xex` gate.
4. If ReXApp's fixed default launch path is too restrictive, switch to a direct
   `rex::Runtime` sidecar only after recording why.

Completion standard:

- Runtime setup reaches post-setup logs.
- `default.xex` loads from the expected `game:` path when explicitly gated.
- Failures are logged as missing content/import/config blockers, not silent
  crashes.
- The default sidecar run still stops before module materialization.
- The gated audit still does not call `LaunchModule()`.

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

1. Commit and push the first-launch native-stack diagnostic boundary.
   Completion standard: only `LibertyRecompRex/src/main.cpp` and audit docs are
   staged, and branch `codex/switch-audit-20260615` is pushed.
2. Investigate `KeSetBasePriorityThread_entry(...)` as the current
   object/native-handle caller.
   Completion standard: prove the raw `thread_ptr` and `increment` values at
   the export boundary, then decide whether a null thread pointer should map to
   the current thread, return an invalid-parameter/status result, or expose a
   missing guest thread-object setup step.
3. If export-boundary logging is insufficient, inspect the generated caller
   chain around `sub_82169578`, `sub_82168C08`, `sub_82167DE0`, and
   `sub_829B3C60` using IDA/ReXGlue metadata.
   Completion standard: record whether the guest intentionally passes zero or
   whether an earlier runtime hook failed to materialize the thread pointer.
4. Keep Vulkan-first work as an explicit later boundary.
   Completion standard: do not enable runtime graphics until the module,
   export, variable-mapping, first-launch failure-capture, video
   null-guard/graphics-system ownership, and object/native-handle
   boundaries are stable.

## New Reference Inputs

- `RPF7-master`: useful later for archive/content research, but it targets the
  RPF7/GTA V AES era and is not part of the current object/native-handle
  blocker.
- `reblue-main`: confirms the desired long-term ReXGlue project shape
  (`generated/rexglue.cmake`, `rexglue_setup_target(...)`, `REX_DEFINE_APP`,
  and a narrow `rex::ReXApp` subclass).
- `ReOdyssey-main`: confirms the same generated/ReXApp shape and adds a useful
  native-renderer ownership example with Plume. Treat it as a renderer/sidecar
  architecture reference after the launch/runtime blockers are stable.

## Entry Condition For Switch Return

Switch should stay paused until the Windows sidecar either:

- reaches the same or better startup boundary than the current legacy Windows
  smoke, or
- proves a hard blocker that must be solved in codegen/content/runtime before
  Switch can benefit.

## 2026-06-20 Update: Host/PPC Register Diagnostic Boundary

- The first-launch sidecar observer now logs AMD64 host registers and the
  active PPC context on the structured exception path.
- Fresh full-root `--audit-launch-module` evidence shows the failing
  `KeSetBasePriorityThread` call reaches the export boundary with
  `r3=0x00000000`, `r4=0x0000000F`, and `lr=0x82169870`.
- Generated source inspection maps `lr=0x82169870` to
  `sub_82169578` immediately after this sequence:
  `ExCreateThread(...) -> ObReferenceObjectByHandle(...) -> KeSetBasePriorityThread(...)`.
- The next Windows/ReXGlue task is no longer a broad threading rewrite. It is a
  narrow `ExCreateThread_entry(...)` / `ObReferenceObjectByHandle_entry(...)`
  diagnostic to prove:
  - which handle/object value `ExCreateThread` writes,
  - whether `ObReferenceObjectByHandle` finds the object,
  - and whether `object->guest_object()` is zero before it is written to the
    guest output slot.
- Fresh verification for this diagnostic boundary:
  `WINDOWS_SMOKE_PASS default=0 load_fullroot=0 launch=8 regs=present ppc_lr=0x82169870 ppc_r3=0 current_rva=0x00045F6A`.
- Do not patch `XObject::GetNativeObject(...)`, generated GTA IV sources, or
  Vulkan/renderer code for this blocker. The evidence points at the
  handle-to-native-object boundary first.

## 2026-06-20 Update: Thread Reference Stack Diagnostic

- The sidecar now captures the generated caller's `sub_82169578` stack slots at
  the first-launch exception without overriding prebuilt `rexkernel` exports.
- Latest evidence:
  `out_thread=0x00000000`, `handle=0xF8000CE0`, `object_found=yes`,
  `object_guest=0x00694018`, `object_thread_id=0x0000000E`.
- Final verification for this diagnostic boundary:
  `WINDOWS_THREAD_REF_SMOKE_PASS default=0 load_fullroot=0 launch=8 current_rva=0x000467AA ... out_thread=0x00000000 ... object_found=yes object_guest=0x00694018`.
- This proves `ExCreateThread(...)` created and registered an `XThread`; the
  output native-thread slot passed to `KeSetBasePriorityThread(...)` remains
  zero.
- The likely root cause is now the `ExThreadObjectType` value:
  the sidecar currently maps ordinal `0x001B` to an allocated guest
  `X_OBJECT_TYPE` structure such as `0x0001B000`, while ReXGlue
  `ObReferenceObjectByHandle_entry(...)` expects the thread dummy type value
  `0xD01BBEEF`.
- Next implementation boundary: use TDD to change only the
  `ExThreadObjectType` sidecar mapping and verify whether launch advances to a
  new blocker. Do not modify generated GTA IV sources or broad ReXGlue kernel
  objects for this boundary.
