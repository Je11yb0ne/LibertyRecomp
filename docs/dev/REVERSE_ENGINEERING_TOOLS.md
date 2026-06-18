# Reverse Engineering Tools

This project may use local reverse engineering tools to inspect user-provided
Xbox 360 game files during development. These tools are not part of the build
toolchain and should not be required for normal users.

## IDA Pro

Local install path:

```text
C:\Program Files\IDA Professional 9.3
```

Useful executables:

```text
C:\Program Files\IDA Professional 9.3\ida.exe
C:\Program Files\IDA Professional 9.3\idat.exe
```

`ida.exe` is useful for interactive analysis. `idat.exe` is useful for batch
IDAPython runs, such as exporting function boundaries, xrefs, strings, vtables,
or switch-table candidates from a user-provided `default.xex`.

Example batch command:

```powershell
& "C:\Program Files\IDA Professional 9.3\idat.exe" -A -S"path\to\script.py" "path\to\default.xex"
```

## IDA MCP

IDA MCP is more convenient for interactive investigation because Codex can ask
IDA for decompilation, disassembly, xrefs, strings, structs, and comments
without writing a new export script each time. Keep `idat.exe` batch scripts
for reproducible exports that should be rerun exactly the same way.

Recommended MCP server:

```text
https://github.com/mrexodia/ida-pro-mcp
```

The preferred path is the headless `idalib-mcp` server, not the older GUI IDA
plugin mode. Codex uses stdio MCP servers cleanly, so configure `idalib-mcp`
with `--stdio`.

Installation status on this Windows machine:

```text
Installed and configured for Codex on 2026-06-15.
```

Setup commands used:

```powershell
python -m pip install --user uv
uv run "C:\Program Files\IDA Professional 9.3\idalib\python\py-activate-idalib.py"
uvx --from git+https://github.com/mrexodia/ida-pro-mcp idalib-mcp --stdio --help
```

Codex user config path:

```text
C:\Users\Jellybone\.codex\config.toml
```

Codex MCP config:

```toml
[mcp_servers.ida]
command = 'C:\Users\Jellybone\AppData\Roaming\Python\Python313\Scripts\uvx.exe'
args = [
    "--from",
    "git+https://github.com/mrexodia/ida-pro-mcp",
    "idalib-mcp",
    "--stdio",
    "--max-workers",
    "2",
]
startup_timeout_sec = 60
```

After editing the Codex config, restart Codex and run `/mcp` to verify that the
`ida` server is listed. New MCP servers normally are not hot-loaded into an
already running Codex session.

Expected uses for LibertyRecomp development:

- Verify PPC function boundaries and call targets.
- Inspect guest addresses seen in runtime logs or crashes.
- Locate global variables, vtables, switch tables, and string references.
- Cross-check generated XenonRecomp/ReXGlue output against original XEX code.
- Export structured analysis data for local tools or config files.

Do not commit IDA databases, license files, or proprietary game assets.

## XenonRecomp / ReXGlue Codegen References

Use ReXGlue as the primary codegen/runtime reference for this project and
XenonRecomp as an auxiliary translation reference.

Useful upstream memory-pressure reference:

```text
https://github.com/OZORDI/XenonRecomp/commit/207253d67cdef67235805d595999fa0a2e4fcbd9
```

The commit removes `static` from per-function scratch containers in
`XenonRecomp/recompiler.cpp`: a labels set, a generated-code string, and a
temporary byte buffer. The issue matters for GTA IV scale because static local
containers keep their capacity across recompilation of many functions; a large
XEX can turn this into unbounded memory growth during codegen.

If a future XenonRecomp/ReXGlue codegen run for GTA IV consumes excessive RAM
or dies after hours, first audit scratch buffers inside hot per-function loops:
`std::string`, `std::vector`, `std::unordered_set`, maps, and similar
containers should usually be ordinary locals or explicitly bounded/reused with
a measured memory policy.
