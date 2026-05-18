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


## A brief description of ResourcePool<T> 


ResourcePool<T> manages a fixed set of reusable resources. It pre-allocates all resources at construction and hands them out as RAII handles, 
when a handle goes out of scope,the resource is automatically returned to the pool, reset to a clean state if the user provided a reset function, 
and made available for the next use.

The implementation focuses on:

1) Thread safety and Synchronization via std::mutex and std::condition_variable
2) Deterministic resource management via Shared State via `shared_ptr<State>`
    The pool share a single `State` object through a `std::shared_ptr`. This was the design decision: the handle needs to reach the pool's queue, mutex, and reset function when it
is destroyed 
3) RAII semantics
4) Move-only ownership
5) Resource reuse without repeated allocations

## Design and Key Decisions
### 1. ResourcePool<T>

The ResourcePool<T> class owns:

A fixed size of resources
Synchronization primitives
Resource availability tracking
Optional reset logic


### 2. RAII-Based Resource Management

Resources are automatically returned when PoolHandle is destroyed.


### 3. no Copy operation, only Move-Only operations

Copy operations are deleted while move operations are supported,as only one handle should own a resource at a time and also to aviod double release and undefined behavior

### 4. Shared Internal State

The pool state is stored in a shared State structure accessed through std::shared_ptr.

To guarantees the internal state remains valid until all handles are destroyed.

### 5. Optional Reset Function

Whenever a resource is returned to the pool, it is reset before becoming available for reuse.

- Users may provide a custom reset callback to define how the resource should be cleaned:

``` [](Resource& r) {
    r.value = 0;
}
```
  Please note that The reset function must not throw. It is called from `release()`, which
  is marked `noexcept` because it runs inside the handle's destructor. If the
  reset function throws, the exception cannot escape the `noexcept` boundary 
  it will be silently swallowed and discarded. The resource will still be
  returned to the pool, but the reset may not have completed cleanly, meaning
  the next caller could receive a partially reset resource. Write your reset
  function defensively

- If no reset callback is provided, the pool falls back to resetting the resource using default construction:
-    ```  else {
            try { *resource_ = T{}; }
            catch (...) {}
        }
        ```
This ensures that reused resources are returned in a predictable and clean state before being acquired again.

### 6. `notify_one()` Called After Releasing the Lock

```cpp
{
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->available.push(index_);
}                          // lock released 
state_->cv.notify_one();   // then notify
```



- Any notable trade-offs or alternatives you considered

1. Shared State via std::shared_ptr

The pool state is stored using std::shared_ptr so that PoolHandle objects can remain valid even if the original ResourcePool object is destroyed.

Trade-off: This adds a small reference-counting overhead, but  improves safety and lifetime management.

Alternative: Using raw pointers or references would be slightly faster, but could lead to dangling references if a handle outlived the pool.

2. blocking synchronization 

The use of mutex and condition variable to block threads when no resources are available.

Trade-off: This design is simple and efficient CPU usage, but blocked threads remain suspended until another thread releases a resource.

Alternative: lock-free or spin-wait but burns CPU, wastes power and much harder.

## A short usage example (code snippet)

```cpp
#include "resource_pool.hpp"

struct Resource {
    int value = 0;
};

int main() {
    //create a pool of 4 connections.
    ResourcePool<Resource> pool(
        4,
        []() { return Resource{}; },          // factory
        [](Resource& c) { c.value = 0; }       // reset 
    );

    //Acquire a handler
    {
        auto h = pool.acquire();
        h->value = 10;
        std::cout << "Using resource " << h->value << "\n";
    } // h goes out of scope,then automatically returned, reset called

    // Timed acquire,will return std::nullopt if no resource free within 100ms.

    auto maybeResource = pool.acquire(std::chrono::milliseconds(100));
    if (maybeResource) {
        std::cout << "Got resource" << (*maybeResource)->value << "\n";
    } else {
        std::cout << "Timed out\n";
    }

    return 0;
}
```

## Known limitations or assumptions

 1) Fixed capacity:The pool size is set at construction and never changes. There is no option to grow or dynamic expansion, but this is also for avoiding runtime allocations and predictable memory usage
 
 2) `notify_one()`: No priority for the waiting thread  some threads may wait longer than others. 
 3)  `T` must be default-constructible if no reset function is provided (the fallback uses `T{}`) but If `T` is not default-constructible, a reset function must be supplied.
