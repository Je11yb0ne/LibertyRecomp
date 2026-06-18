# ReXGlue Reference Review

## Scope

Review local ReXGlue-based reference projects before making the next LibertyRecomp / GTA IV runtime change.

Primary refs:

- `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\skate3recomp`
- `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TheOutFit`
- `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\bo2-recompiled`
- `C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TDURE`

Current project baseline:

- LibertyRecomp already compiles ReXGlue-generated GTA IV sources through `LibertyRecompLib`.
- The app shell is still a legacy LibertyRecomp startup/runtime shell, not a clean ReXGlue `ReXApp` consumer.
- This review must separate adoptable patterns from patterns that depend on project-specific forks or finished game-specific reverse engineering.

## Questions

1. What does a complete ReXGlue project do differently from the current LibertyRecomp hybrid?
2. Which parts are needed to get Windows to a visible GTA IV frame faster?
3. Which parts matter for a later Switch path?
4. Which refs provide reliable patterns for content install/layout, VFS, generated source handling, import overrides, runtime app lifecycle, graphics/audio/input, and title update/DLC handling?

## Findings

### skate3recomp

Path:

`C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\skate3recomp`

High-level shape:

- This is a complete ReXGlue-style app, not a legacy bootstrap with generated code linked on the side.
- The executable target links `rex::runtime`, calls `rexglue_configure_target(skate3)`, and uses `REX_DEFINE_APP(skate3, Skate3PureApp::Create)`.
- `Skate3BaseApp` derives from `rex::ReXApp` and owns lifecycle hooks such as path configuration, install checks, dialog setup, post-setup runtime hooks, shutdown, and content overlays.
- Generated sources are attached to the app target only when `generated/sources.cmake` and `generated/eawebkit/sources.cmake` exist.
- Codegen is a first-class CMake workflow: manifest templates are configured into the build tree, `generate-skate3`, `generate-eawebkit`, and `generate-all` run `rex::rexglue codegen`, and generated-source patches run as explicit codegen post-steps.

Codegen/content model:

- The project maintains manifest templates for both the main `default.xex` and a secondary `EAWebkit.xex` module.
- Title-update payloads are staged before codegen so patched XEX/XEXP inputs produce the generated output.
- Function-boundary TOML files are large and explicit. This supports the conclusion that mature ReXGlue projects curate codegen/function metadata instead of relying only on runtime patching.
- `.gitmodules` pins a game-specific ReXGlue SDK fork (`mchughalex/rexglue-skate3`, branch `skate3-sdk-clean`). Treat fork behavior as reference material, not directly portable API.

Runtime/content model:

- First-run/content handling lives in app lifecycle, especially `OnFinalizePaths()` and `OnPostSetup()`, not in ad hoc startup code.
- The ISO installer extracts and validates `default.xex` before runtime startup.
- The title-update installer validates known `.xexp` payloads by size and hash and can read STFS package containers.
- DLC handling uses ReXGlue runtime services such as `kernel_state()->content_manager()->InstallContent()`.
- VFS overlays use ReXGlue filesystem APIs such as `HostPathDevice` and `RegisterSymbolicLink()` for BIG-directory aliases and guest-path overlays.

Override/import model:

- Project-side wrappers call generated `__imp__` implementations when wrapping generated functions. `exception_compat.cpp` is the clearest example.
- Some game-specific behavior is injected through `runtime()->function_dispatcher()->SetFunction(...)`.
- The project also applies targeted generated-source patches for ultrawide/FOV changes after codegen. That is a deliberate post-codegen patch step with anchor validation, not an accidental edit to generated code.

Adoptable for LibertyRecomp / GTA IV:

- Add a ReXGlue-aligned app-shell plan instead of treating the current legacy `LibertyRecomp/main.cpp` as the final runtime shape.
- Keep using generated `__imp__sub_x` implementations as source of truth for wrappers.
- Move future content/install checks toward a lifecycle-owned path like `OnFinalizePaths()` once a ReXApp target exists.
- Prefer explicit codegen manifests/function-boundary curation for persistent fixes; use runtime wrappers only for verified platform/game glue.
- Use ReXGlue VFS/device/symlink concepts as the target design for Windows first, then map that model to Switch packaging/content constraints.

Not directly portable:

- The Skate 3 ISO/XDVDFS installer does not solve GTA IV encrypted RPF2 content or the missing `aes_key.bin` problem.
- The Skate-specific ReXGlue SDK fork may contain runtime behavior not present in the SDK currently vendored by LibertyRecomp.
- Generated-source ultrawide/FOV patches are game-specific and should not be copied; only the validated post-codegen patch pattern is reusable.
- The desktop ReXApp UI/install wizard is not a Switch solution by itself. Switch still needs a separate user-facing content/install/error policy.

### TheOutFit

Path:

`C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TheOutFit`

High-level shape:

- This project uses a ReXGlue-generated CMake helper: `TheOutFit_Port/generated/rexglue.cmake`.
- The hand-maintained `CMakeLists.txt` stays small: it includes `generated/rexglue.cmake`, adds app sources, then calls `rexglue_setup_target(theoutfit)`.
- `rexglue_setup_target()` pulls in generated sources when present, links `rex::runtime`, and calls `rexglue_configure_target()`.
- `main.cpp` is tiny and uses `REX_DEFINE_APP(theoutfit, TheoutfitApp::Create)`.
- `TheoutfitApp` derives from `rex::ReXApp` and uses lifecycle hooks for path autodetection, guest-thread launch diagnostics, shutdown diagnostics, and a watchdog thread.

Codegen/debug model:

- The manifest is the modern ReXGlue style: `[project]`, `[entrypoint]`, `game_root`, `file_path`, `out_directory_path`, and `includes = ["config/manual_functions.toml"]`.
- `config/manual_functions.toml` records manual function seeds and a manual `[[switch_tables]]` entry, with comments tying entries to Ghidra MCP or fallback generated-code evidence.
- Local SDK patch documentation is explicit. Required/important patterns include:
  - making manual switch-table labels participate in block discovery,
  - accepting modifier-only physical-memory protections,
  - keeping D3D12/GPU changes classified as diagnostics unless A/B evidence proves a fix.
- The docs keep address evidence, toolchain history, and regression logs next to the project. This is the right shape for GTA IV blocker notes too.

Adoptable for LibertyRecomp / GTA IV:

- Create an evidence ledger for GTA IV manual functions/switch tables instead of burying discoveries in chat or one-off source comments.
- Treat manual switch-table/function-boundary data as codegen input first, not as runtime bandaids.
- Use a ReXApp watchdog/guest-thread diagnostic style when running Windows bring-up, especially once the current legacy bootstrap is replaced or wrapped by a ReXApp target.
- Keep SDK/runtime patches classified as required, diagnostic-only, or failed experiment. Do not let speculative renderer/runtime patches become permanent without smoke evidence.

Not directly portable:

- TheOutFit is Windows-focused and currently documents D3D12-specific renderer issues; it does not solve Switch rendering.
- Its generated output is intentionally ignored and recreated locally, while LibertyRecomp currently tracks generated GTA IV sources under `glue/rexglue-sdk-main/gta4-recomp/generated`.
- The Ghidra MCP notes confirm the value of interactive RE, but the concrete addresses and switch tables are game-specific.

### bo2-recompiled

Path:

`C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\bo2-recompiled`

High-level shape:

- The repo splits single-player and multiplayer into separate ReXGlue projects:
  - `default` for `default.xex`
  - `default_mp` for `default_mp.xex`
- Each project includes its own generated `rexglue.cmake`, creates a small executable, then calls `rexglue_setup_target(<target>)`.
- Both entrypoints use `REX_DEFINE_APP(...)` and small `ReXApp` subclasses.
- Build scripts make codegen/build/preset workflows explicit for `sp`, `mp`, or `both`.

Runtime model:

- `OnConfigurePaths()` sets `update_data_root = game_data_root`, which is a simple but useful pattern for games that expect update data to resolve beside base content during bring-up.
- `OnPreSetup()` sets runtime cvars such as `vsync` and `ignore_thread_priorities`.
- `default_mp` overrides `OnLoadXexImage()` to launch `game:/default_mp.xex`.
- `default_mp` uses `OnPostSetup()` to install XAM/network overrides through `runtime()->function_dispatcher()->SetFunction(...)`.
- Its XAM override layer includes both dispatcher registrations and Win32 host detours for generated `__imp__` imports. This is aggressive and Windows-specific, but it shows a mature ReXGlue project keeping import overrides explicit and centralized.
- The project still carries many import stubs. A "playable" ReXGlue project can still have game-specific or service-specific stubs; stubs must be classified rather than treated as automatic failure.

Adoptable for LibertyRecomp / GTA IV:

- If GTA IV later needs EFLC/alternate module handling, keep entrypoints/modules explicit instead of making one ambiguous executable path.
- Centralize import overrides by subsystem and install them in `OnPostSetup()` or a comparable lifecycle hook.
- Use runtime cvar setup to force deterministic bring-up conditions.
- Keep stubs in a ledger with behavior and completion criteria; this matches the existing Switch `SWITCH_STUBS.md` pattern.

Not directly portable:

- The BO2 Win32 `CreateProcessW()` handoff and host detour patching are not Switch-friendly.
- Network/session overrides are tailored to BO2 and Xbox Live behavior.
- It does not provide a GTA IV RPF2 content solution.

### TDURE

Path:

`C:\Users\Jellybone\Documents\Codex\2026-06-12\d-gta4-ns\work\refs\TDURE`

High-level shape:

- This is closest to the minimal ReXGlue template:
  - `include(generated/rexglue.cmake)`
  - app executable with `src/main.cpp`
  - `rexglue_setup_target(testrec)`
  - `REX_DEFINE_APP(testrec, TestrecApp::Create)`
  - `TestrecApp : rex::ReXApp`
- The config is old/simple TOML with a few manual function ranges.
- Generated sources are present locally under `generated/`.

Adoptable for LibertyRecomp / GTA IV:

- Useful as a sanity check for the minimal ReXGlue app shape.
- It demonstrates that the clean ReXGlue app shell can be very small when the runtime/config are aligned.

Not directly portable:

- It lacks the mature installer/content/override patterns needed by GTA IV.
- It is not enough evidence for graphics, RPF, guest memory, or Switch decisions.

### Cross-reference conclusion

All four refs point in the same direction:

- A clean ReXGlue project treats `ReXApp` as the app shell.
- Generated code is included through ReXGlue-generated or ReXGlue-aware CMake helpers.
- The app target links `rex::runtime` and is configured with `rexglue_configure_target()` or the generated `rexglue_setup_target()` wrapper.
- Content paths and install checks live in ReXApp lifecycle hooks.
- Import overrides are installed deliberately through the runtime dispatcher, hooks, or strongly named wrappers.
- Manual functions and switch tables are durable codegen inputs with evidence, not random source edits.

Current LibertyRecomp / GTA IV differs in important ways:

- GTA IV ReXGlue-generated sources exist and build, but they are wrapped into `LibertyRecompLib` rather than attached to a clean ReXGlue app target.
- `LibertyRecomp/main.cpp` is still a legacy LibertyRecomp bootstrap with Switch audit gates, not a ReXApp entrypoint.
- `LibertyRecomp/app.cpp` is still placeholder-style glue and does not define the real ReXGlue application lifecycle.
- Switch currently depends on many audit stubs because the full ReXGlue runtime is not linked/portable there yet.
- Windows bring-up is currently fixing legacy wrapper/import/runtime mismatches one blocker at a time, which explains why progress feels slower than a project started from ReXGlue templates.

## Current Recommendation

Do not continue broad runtime hacking before choosing a ReXGlue alignment path.

Recommended next path:

1. Keep the current legacy LibertyRecomp executable as the short-term Windows smoke harness.
2. Start a separate ReXGlue-aligned Windows prototype target for GTA IV, using the existing generated `PPCImageConfig` / generated sources and a minimal `Gta4ReXApp`.
3. Do not move Switch to that prototype until Windows proves the app shell can reach the same or better bring-up boundary.
4. Keep Switch frozen at the LibertyRecompExeFs / NSP-like pre-guest baseline except for regression checks.
5. Continue content work as a separate boundary: GTA IV still needs a real loose/RPF2 content preparation plan before many file-open misses can resolve.

Why this path:

- The refs show that the fastest route to a visible game frame is usually ReXGlue-native app structure plus curated codegen/runtime overrides, not endlessly adapting the old shell.
- A sidecar/prototype target avoids breaking the current Switch audit baseline and avoids a risky all-at-once migration.
- Windows remains the right place to prove ReXGlue app/runtime/content behavior before translating the result back toward Switch constraints.
