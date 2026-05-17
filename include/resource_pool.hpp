#pragma once
#include <vector>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <functional>
#include <utility>
#include "stdexcept"
#include <optional>
#include <mutex>
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
    // Default constructor needed for R7 and move assignment
    PoolHandle() = default;

    // user defined constructor
    struct State {
        std::vector<T> resources; //resource storage
        std::queue<std::size_t> available; // free slots
        std::mutex mutex; //R5
        std::condition_variable cv;//to blocks acquire until a slot is free (R2)
        std::function<void(T&)> reset; ///cleanup "optional"(R6)
    };

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
    ~PoolHandle() { release(); } //R4:The RAII handle releases its resource automatically on destruction
    // TODO: operator* and operator->.

    //non const versions 
    T& operator*() { return *resource_; }

    T* operator->() { return resource_; }

    // const versions 
    const T& operator*() const { return *resource_;}

    const T* operator->() const { return resource_;}

private:

    void release() noexcept {
        if (!state_ || !resource_) { return;}

        if (state_->reset) {
            try { state_->reset(*resource_); }
            catch (...) {}
        }
       
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->available.push(index_);
        state_->cv.notify_one();

        resource_ = nullptr;
        index_ = 0;
    }


    friend class ResourcePool<T>;
    // TODO: Store a pointer/reference to the resource and back-reference to the pool.
    std::shared_ptr<State> state_;
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
    using Handler = PoolHandle<T>;

    // TODO: Constructor.
    //
    explicit ResourcePool(
         std::size_t capacity,
         std::function<T()> factory,
         std::function<void(T&)> reset = {}) 
        : state_(std::make_shared<typename Handler::State>()) {
        if (capacity == 0) {throw std::invalid_argument(" capacity should be greater then 1");}

        state_->reset = std::move(reset);
        state_->resources.reserve(capacity);
        for (std::size_t i = 0; i < capacity; ++i) 
        {
            state_->resources.push_back(factory());
            if (state_->reset) {state_->reset(state_->resources.back());}
            state_->available.push(i);
        }
    }

    // Non-copyable and non-movable (owning resource collection).
    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;

    // TODO: Destructor — document and implement your chosen destruction semantics.
    ~ResourcePool() = default;

    // TODO: PoolHandle<T> acquire();

    Handler acquire() {
        std::unique_lock<std::mutex> lock(state_->mutex);

        state_->cv.wait(lock, [this] {return !state_->available.empty();});

        const std::size_t index = state_->available.front();
        state_->available.pop(); 

        return Handler(state_, &state_->resources[index], index);
    }

    // TODO: std::optional<PoolHandle<T>> acquire(/* timeout duration */);

    }

private:
    std::shared_ptr<typename Handler::State> state_;
};
