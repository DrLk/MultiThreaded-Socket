---
allowedTools:
  - Edit
  - Read
---

Run clang-tidy on all C/C++ files changed vs master, auto-fix all warnings, then run clang-format on those same files. Uses the local Docker image `clang-tidy-image`.

## Steps

1. **Get changed files and resolve host path:**
   ```bash
   /workspace/.claude/scripts/tidy-get-files.sh
   ```
   Parse `HOST_SRC=<path>` from the first line of output. Collect the remaining lines as the file list.
   If there are no files, report that and stop.

2. **Build compile_commands.json, run clang-tidy --fix, and clang-format:**
   ```bash
   /workspace/.claude/scripts/tidy-fix.sh <HOST_SRC> <FILES>
   ```

3. **Verify — re-run clang-tidy without --fix** to collect any remaining diagnostics:
   ```bash
   /workspace/.claude/scripts/tidy-verify.sh <HOST_SRC> <FILES>
   ```

3a. **Fix remaining errors manually** — for each remaining clang-tidy warning/error:
   - Read the affected file
   - Understand the diagnostic and fix it directly using the Edit tool
   - Re-run `tidy-verify.sh` on that file to confirm the fix
   - Repeat until all diagnostics are resolved
   - Only ask the user for guidance if you genuinely cannot determine the correct fix (e.g. ambiguous design decision, missing context, or a change that could break unrelated code).

4. **Report results** — show which files were modified and summarize any remaining issues.

## Notes
- The Docker image is `clang-tidy-image` (built from `clang-tidy-docker/`). If it's not present, tell the user to build it first: `cd clang-tidy-docker && ./builddocker.sh`
- Claude Code runs inside a container, so `/workspace` is not a valid host path for Docker bind mounts. `tidy-get-files.sh` resolves the real host path automatically.
- The repo root is mounted at `/build` inside the container
- File paths passed to clang-tidy/clang-format inside the container must use the `/build/` prefix
