# GitHub Backup 2026-06-15

This directory records submodule working-tree diffs that a normal superproject
commit cannot store directly.

Snapshot branch:

`codex/github-backup-20260615-022231`

Main snapshot commit:

`f2fe4cb4eb6a08b71cb1bf437606cdf8e5712e27`

Apply a submodule patch from the repository root with:

```powershell
git -C <submodule-path> apply ..\..\docs\backups\github-backup-20260615-022231\submodule-diffs\<patch-file>
```

Adjust the relative path if applying from a different current directory.
