# MultiThreaded-Socket

High-performance custom transport protocol over UDP with a FUSE3-based remote filesystem (Linux). Written in C++23.

## Repository Structure

```
/
├── SimpleNetworkProtocol/     # Main project (C++23, CMake)
├── MS Network API test/       # Legacy Windows prototype using WinSock select API
├── clang-tidy-docker/         # Docker image definition (clang-21, g++-15, Ubuntu 25.10)
├── build/                     # CMake build output (Debug, RelWithDebInfo)
└── readme/                    # HTML documentation with images
```

### MS Network API test/

Early Windows prototype of the network API built on WinSock2 `select()`. Contains `IPacket`/`IPipe` interfaces and `SelectPipe`/`SelectWriteThread` implementations. Superseded by `SimpleNetworkProtocol/`.

## Build & Workflow

All commands run inside the `clang-tidy-image` Docker container (repo root mounted at `/build`).

| Task | Command |
|------|---------|
| Configure (Debug) | `/generate-project` |
| Configure (Release) | `/generate-project-release` |
| Build | `/build` |
| Run tests | `/test` |
| Lint & format | `/tidy` |

Run `/tidy` before committing — it auto-fixes clang-tidy warnings and applies clang-format.

## Project Structure

```
SimpleNetworkProtocol/
├── Protocol/                  # Job-scheduled pipeline bridging network and filesystem
├── SimpleNetworkProtocolLib/  # Core transport library: connections, queues, congestion control, serialization
├── FileSystem/                # FUSE3 remote filesystem (Linux only)
├── Packet/                    # IPacket abstraction — header types, seq numbers, payload
├── Logger/                    # Thread-safe logging (LOGGER macro, timestamps, stdout)
├── Memory/                    # PMR-compatible pool allocators (per-thread + global)
├── LockedList/                # Thread-safe MultiList with blocking wait / splice semantics
├── test/                      # Google Test files (auto-discovered by CMake)
└── external/                  # googletest, tracy (git submodules)
```

### Protocol/

Orchestrates network I/O and filesystem operations via a worker queue. Key classes: `Job` (base task), `MainJob` (executable work unit), `TaskScheduler` (dispatcher), `NetworkStream` (I/O wrapper), plus specialized job handlers for filesystem requests and file caching. Bridges `SimpleNetworkProtocolLib` and `FileSystem`.

### SimpleNetworkProtocolLib/

Core reliable UDP transport library. `FastTransportContext` owns all connections and runs send/recv threads. Each `Connection` is a state machine (`SendingSyn → WaitingSynAck → Data → Closing → Closed`) with three queues: `SendQueue` (outgoing), `RecvQueue` (reorder buffer, 250k capacity), `InFlightQueue` (unacked packets + congestion control). `SpeedController` implements AIMD with three states: Fast (probe), BBQ (post-loss), Stable (fallback). `MessageWriter`/`MessageReader` provide `<<`/`>>` streaming over packet lists; `ConnectionWriter`/`ConnectionReader` add async/blocking wrappers on top.

### FileSystem/

FUSE3 daemon (Linux only) that mounts a remote volume at `/mnt/test`. Handles directory traversal, metadata, and piece-based cached reads/writes with hash verification. Key classes: `FileSystem` (FUSE daemon), `FileCache` (LRU file handle cache), `File`.

### Packet/

Defines `IPacket` — the fundamental network unit. Carries header fields (magic, connection IDs, seq/ack numbers, packet type) and payload. Packets are managed in `MultiList` containers and pooled for reuse throughout the stack.

### Logger/

Header-only, macro-based logging (`LOGGER << ...`). Writes timestamped lines to stdout with a mutex-protected sync stream. Uses stack-allocated buffers via `std::pmr` to avoid heap allocations on the hot path.

### Memory/

Custom `std::pmr::memory_resource` implementations: `PoolAllocator` (global, thread-safe) and `LocalPoolAllocator` (per-thread, lock-free). Used throughout the stack to avoid repeated `malloc`/`free` calls.

### LockedList/

`LockedList<T>` — a `MultiList` wrapped with a mutex and condition variable. Supports blocking `wait`, atomic push/pop, and O(1) splice for producer-consumer handoff between protocol threads.

## CMake Options

| Option | Default | Purpose |
|--------|---------|---------|
| `ENABLE_PRECOMPILE_HEADERS` | ON | Speed up builds |
| `ENABLE_TRACY` | OFF | Tracy profiler integration |
| `ENABLE_TSAN` | OFF | Thread sanitizer for tests |
| `ENABLE_TESTS` | ON | Build test binary |

## Code Style

- **Formatter:** clang-format, WebKit base style, 4-space indent, no column limit, pointer-left (`int* p`)
- **Linter:** clang-tidy with all checks enabled, warnings treated as errors
  - Key disabled checks: `magic-numbers`, `trailing-return-type`, `misc-include-cleaner`, `cognitive-complexity`, `bugprone-easily-swappable-parameters`
- `UNIT_TESTING=1` is defined in test builds

## Architecture

- **Linux-only:** FileSystem and Protocol modules; Windows builds only SimpleNetworkProtocolLib
- **Data flow:** `ConnectionWriter → MessageWriter → SendQueue → InFlightQueue → UDPQueue → Socket` (write); reverse on read
