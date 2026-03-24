---
allowedTools:
  - Bash
---

Run the built `SimpleNetworkProtocolTest` binary inside the local Docker container (`clang-tidy-image`).

## Steps

1. **Run tests:**
   ```bash
   .claude/scripts/test.sh $ARGUMENTS
   ```

2. **Report results:**
   - On success: confirm all tests passed.
   - On failure: show the failed tests clearly and stop. Do not attempt to fix failures unless the user asks.

## Notes
- If `$ARGUMENTS` is provided it is used as `--gtest_filter`.
- Run `/build` first if the binary doesn't exist yet.
- The Docker image is `clang-tidy-image`. The container gets `--device /dev/fuse` and `--cap-add SYS_ADMIN` for FUSE support.
- The repo root is mounted at `/build` inside the container.
