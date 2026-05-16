# Resource Pool — C++ Home Assignment

## Overview

This repository contains a C++ home assignment. Read [`TASK.md`](TASK.md) carefully before starting.

## Repository Structure

```
include/resource_pool.hpp     ← Your implementation goes here
tests/test_resource_pool.cpp  ← Your tests go here (skeletons provided)
CMakeLists.txt                ← Build configuration (set your C++ standard here)
TASK.md                       ← Full task specification
README.md                     ← This file — replace with your design notes
```

## Build and Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires: CMake ≥ 3.20, a C++17/20-capable compiler, internet access (GTest is fetched automatically).

Recommended: also run with ThreadSanitizer to verify your concurrent stress test:

```bash
cmake -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan
ctest --test-dir build-tsan --output-on-failure
```

## Submission

1. Replace this `README.md` with your design notes (see `TASK.md` for what to include).
2. Share your private repository with `<your-recruiter-contact>`.
3. Ensure `cmake --build build && ctest --test-dir build` passes from a clean checkout.
