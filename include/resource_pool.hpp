#pragma once

// TODO: Add necessary standard library includes.

// -----------------------------------------------------------------------------
// Forward declaration
// -----------------------------------------------------------------------------

template <typename T>
class ResourcePool;

// -----------------------------------------------------------------------------
// PoolHandle<T>
//
// RAII handle returned by ResourcePool<T>::acquire().
//
// Requirements:
//   - Move-only (implement move constructor and move assignment; delete copy).
//   - Destructor returns the resource to the originating pool.
//   - operator* and operator-> provide access to the underlying T.
//   - A moved-from handle must be safely destructible (no double-release).
// -----------------------------------------------------------------------------

template <typename T>
class PoolHandle {
public:
    // TODO: Implement.

    PoolHandle(const PoolHandle&) = delete;
    PoolHandle& operator=(const PoolHandle&) = delete;

    // TODO: Move constructor.
    // TODO: Move assignment operator.

    // TODO: Destructor — return the resource to the pool.

    // TODO: operator* and operator->.

private:
    // TODO: Store a pointer/reference to the resource and back-reference to the pool.
};

// -----------------------------------------------------------------------------
// ResourcePool<T>
//
// A fixed-capacity, thread-safe pool of reusable resources.
//
// Requirements (see TASK.md for full specification):
//   R1 - All resources pre-allocated at construction.
//   R2 - acquire() blocks until a resource is available.
//   R3 - acquire(timeout) returns empty optional on timeout.
//   R4 - RAII handle releases automatically on destruction.
//   R5 - All operations are thread-safe without external synchronization.
//   R6 - reset function (if provided) called on every release before reuse.
//   R7 - Moved-from handle is in a safe, destructible state.
//   R8 - Pool destruction behavior is defined, safe, and documented.
// -----------------------------------------------------------------------------

template <typename T>
class ResourcePool {
public:
    // TODO: Constructor.
    //
    // explicit ResourcePool(
    //     std::size_t capacity,
    //     std::function<T()> factory,
    //     std::function<void(T&)> reset = {}
    // );

    // Non-copyable and non-movable (owning resource collection).
    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;

    // TODO: Destructor — document and implement your chosen destruction semantics.

    // TODO: PoolHandle<T> acquire();

    // TODO: std::optional<PoolHandle<T>> acquire(/* timeout duration */);

private:
    // TODO: Storage for resources, synchronization primitives, and reset function.
};
