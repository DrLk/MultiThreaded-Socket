---
allowedTools:
  - Bash
---

Build the project using ninja inside the local Docker container (`clang-tidy-image`). Run `/generate-project` or `/generate-project-release` first if the build directory doesn't exist yet.

## Steps

1. **Run ninja:**
   ```bash
   .claude/scripts/build.sh build
   ```

2. **Report results:**
   - On success: confirm the build completed successfully.
   - On failure: show the errors clearly and stop. Do not attempt to fix build errors unless the user asks.

## Notes
- The Docker image is `clang-tidy-image` (built from `clang-tidy-docker/`). If it's not present, tell the user to build it first: `cd clang-tidy-docker && ./builddocker.sh`
- The repo root is mounted at `/build` inside the container
