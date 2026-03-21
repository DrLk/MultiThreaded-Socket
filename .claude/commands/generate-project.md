---
allowedTools:
  - Bash
---

Configure the project in Debug mode using cmake inside the local Docker container (`clang-tidy-image`).

## Steps

1. **Run cmake:**
   ```bash
   .claude/scripts/build.sh generate
   ```

2. **Report results:**
   - On success: confirm configuration completed.
   - On failure: show the errors clearly and stop.

## Notes
- The Docker image is `clang-tidy-image` (built from `clang-tidy-docker/`). If it's not present, tell the user to build it first: `cd clang-tidy-docker && ./builddocker.sh`
- The repo root is mounted at `/build` inside the container
