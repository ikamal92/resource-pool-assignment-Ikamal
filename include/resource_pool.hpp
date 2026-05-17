#pragma once
#include <vector>
#include <condition_variable>
#include <queue>

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
    // Default constructor needed for move assignment
    PoolHandle() = default;

    // user defined constructor
    struct State {
        std::vector<T> resources;
        std::queue<std::size_t> available;
        std::mutex mutex;
        std::condition_variable cv;
    };

    std::shared_ptr<State> state_;

    PoolHandle(std::shared_ptr<State> state, T* resource, std::size_t index)
        : state_(std::move(state))
        , resource_(resource)
        , index_(index) {
    }

    PoolHandle(const PoolHandle&) = delete;
    PoolHandle& operator=(const PoolHandle&) = delete;

    // TODO: Move constructor.
    PoolHandle(PoolHandle&& other) noexcept
        : state_(std::move(other.state_))
        , resource_(other.resource_)
        , index_(other.index_) {}

    // TODO: Move assignment operator.
    PoolHandle& operator=(PoolHandle&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
            resource_ = other.resource_;
            index_ = other.index_;
        }

        return *this;
    }
    // TODO: Destructor — return the resource to the pool.
    ~PoolHandle() {}
    // TODO: operator* and operator->.

    //non const versions 
    T& operator*() { return *resource_; }

    T* operator->() { return resource_; }

    // const versions 
    const T& operator*() const { return *resource_;}

    const T* operator->() const { return resource_;}

private:
    void release()  {//TODO
    }
    // TODO: Store a pointer/reference to the resource and back-reference to the pool.
    T* resource_ = nullptr;
    std::size_t index_ = 0;

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
