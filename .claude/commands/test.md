---
allowedTools:
  - Bash
---

Run the built `SimpleNetworkProtocolTest` binary.

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
