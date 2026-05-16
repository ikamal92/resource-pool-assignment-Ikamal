# C++ Home Assignment: Thread-safe Resource Pool

## Background

Platform software for robotics systems frequently manages scarce, reusable resources: network connections,
device handles, pre-allocated memory buffers, hardware communication channels. A resource pool allows
multiple concurrent consumers to share a fixed set of such resources safely without reallocation.

Your task is to implement a generic, thread-safe resource pool in modern C++.

---

## What to Build

Implement a `ResourcePool<T>` class template and its accompanying RAII handle type.

### `ResourcePool<T>`

A fixed-capacity pool of objects of type `T`.

**Constructor**

```cpp
ResourcePool(
    std::size_t capacity,
    std::function<T()> factory,
    std::function<void(T&)> reset = {}
);
```

- `capacity`: the number of resources pre-allocated at construction. Must be ≥ 1.
- `factory`: called exactly `capacity` times during construction to create the resources.
- `reset`: if provided, called on a resource each time it is returned to the pool, before it becomes available again.

**Acquire (blocking)**

```cpp
Handle acquire();
```

Blocks the calling thread until a resource is available, then returns an RAII handle to it.
Must be safe to call concurrently from multiple threads.

**Acquire (timed)**

```cpp
std::optional<Handle> acquire(std::chrono::duration<...> timeout);
```

Blocks up to `timeout`. Returns a populated `optional<Handle>` if a resource became available in time,
or an empty `optional` if the timeout expired.
The exact duration type is your design decision — document your choice.

**Destruction**

Define and document what happens when the pool is destroyed while handles are still outstanding.
Your chosen behavior must be safe (no crashes, no undefined behavior) and consistent.
State your decision clearly in the README.

### RAII Handle

The type returned by `acquire()`. It must:

- Be **move-only** (non-copyable).
- Automatically return the resource to the pool when destroyed.
- Provide `operator*` and `operator->` to access the underlying `T`.
- Be safe to destroy after being moved from (no double-release, no crash).

---

## Requirements Summary

| # | Requirement |
|---|---|
| R1 | All resources are pre-allocated at `ResourcePool` construction |
| R2 | `acquire()` blocks until a resource is available |
| R3 | `acquire(timeout)` returns empty `optional` on timeout |
| R4 | The RAII handle releases its resource automatically on destruction |
| R5 | All operations are thread-safe without external synchronization |
| R6 | The reset function (if provided) is called on every release, before the resource is reused |
| R7 | A moved-from handle is in a safe, destructible state |
| R8 | Pool destruction behavior is defined, safe, and documented |

---

## Constraints

- **Standard library only.** No Boost, no third-party concurrency libraries.
- **C++17 or C++20** — your choice. Set the standard explicitly in `CMakeLists.txt`.
- **CMake** as the build system. Tests must be runnable via `ctest`.
- **GTest** for unit tests. Fetching it via `FetchContent` in CMake is fine.
- Compile with **`-Wall -Wextra -Werror`**. Your code must compile cleanly.
- **No AI assistance.** [cppreference.com](https://en.cppreference.com) is permitted.

---

## What to Write

### Implementation

Place your implementation in `include/resource_pool.hpp` (header-only is fine; you may add `.cpp` files if you prefer).

### Tests

Write GTest unit tests in `tests/test_resource_pool.cpp`. At minimum, cover:

1. Basic acquire and release
2. Pool exhaustion — a blocked `acquire()` unblocks after another thread releases
3. Timed `acquire` returns empty `optional` when timeout expires
4. RAII: resource is returned automatically when the handle goes out of scope
5. Concurrent stress test: ≥4 threads repeatedly acquiring and releasing
6. Reset function is called on every release
7. A moved-from handle is safely destructible

You are encouraged to add more tests if you see additional edge cases worth covering.

### README

Write a `README.md` (1–2 pages) that includes:

- A brief description of your design and the key decisions you made
- Any notable trade-offs or alternatives you considered
- A short usage example (code snippet)
- Known limitations or assumptions

---

## Submission

1. Create a **private Git repository** (GitHub, GitLab, or Bitbucket).
2. Commit your work incrementally — every step should be committed so reviewers can understand what was implemented.
3. Share repository access with `<your-recruiter-contact>`.
4. Include a top-level `README.md` with build instructions and your design notes.

---

## Building and Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
