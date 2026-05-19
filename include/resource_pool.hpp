#pragma once

#include <vector>             // std::vector
#include <condition_variable> // std::condition_variable 
#include <queue>              // std::queue
#include <chrono>             // std::chrono::duration
#include <functional>         // std::function
#include <utility>            // std::move
#include <stdexcept>          // std::invalid_argument
#include <optional>           // std::optional
#include <mutex>              // std::mutex
#include <memory>             // std::shared_ptr 
#include <cstddef>            // std::size_t 


// -----------------------------------------------------------------------------
// Forward declaration
// -----------------------------------------------------------------------------
// PoolHandle<T> references ResourcePool<T> as a friend so ResourcePool can
// call PoolHandle's private constructor so Forward declaration used 
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

    // Default constructor needed for moved-from handles are left in a valid state (R7).
    // gives a handle that owns nothing. All pointers are null.
    PoolHandle() = default;

    // Copy deleted R4, R7
    // As copying a handle would give two owners of the same slot.
    PoolHandle(const PoolHandle&) = delete;
    PoolHandle& operator=(const PoolHandle&) = delete;

    // Move constructor R7
    //  'noexcept' as Move constructors should never throw. Marking noexcept allows <PoolHandle> to use the move
    //   constructor instead of falling back to copy.
    PoolHandle(PoolHandle&& other) noexcept
        : state_(std::move(other.state_))
        , resource_(other.resource_)
        , index_(other.index_) {}

    // Move assignment (R7)
    PoolHandle& operator=(PoolHandle&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
            resource_ = other.resource_;
            index_ = other.index_;
            other.resource_ = nullptr;
            other.index_ = 0;
        }

        return *this;
    }
    // Destructor (R4) The RAII handle releases its resource automatically on destruction
    ~PoolHandle() { release(); } 


    // non const versions 
    // caller can modify the resource
    T& operator*() { return *resource_; }

    T* operator->() { return resource_; }

    // const versions   
    // caller can only READ
    const T& operator*() const { return *resource_;}

    const T* operator->() const { return resource_;}

private:
    // friend declaration
    friend class ResourcePool<T>;

    // State — the shared bridge between pool and handles
    struct State {
        std::vector<T> resources; // resource storage
        std::queue<std::size_t> available; // free slots
        std::mutex mutex; // Protects 'available' (R5)
        std::condition_variable cv; // to blocks acquire until a slot is free (R2)
        std::function<void(T&)> reset; // cleanup "optional" (R6)
    };

    // Private (user defined) constructor, only called by ResourcePool::acquire()
    PoolHandle(std::shared_ptr<State> state, T* resource, std::size_t index)
        : state_(std::move(state))
        , resource_(resource)
        , index_(index) {
    }

    // release() — return the resource to the pool
    // noexcept: required because this is called from the destructor.
    void release() noexcept {
        if (!state_ || !resource_ || index_ == static_cast<std::size_t>(-1)) { return;}

        if (state_->reset) {
            try { state_->reset(*resource_); }
            catch (...) {}
        }
        else {
            try { *resource_ = T{}; }
            catch (...) {}
        }
        // Return the slot index to the available queue (R5) 
        {
          std::lock_guard<std::mutex> lock(state_->mutex);
          state_->available.push(index_);

        }

        // wake one thread that's waiting for a slot (R2).
        state_->cv.notify_one();

        // ── Invalidate this handle (R7) ───────────────────────────────────────
        // Null out pointer and set indez to -1 as if release() is 
        // called again like from destructor of a moved-from handle, the guard at the top of the func exits immediately then no double-release.
        resource_ = nullptr;
        index_ = static_cast<std::size_t>(-1);
    }

    // Data members
    std::shared_ptr<State> state_; // shared ownership of internal state.
    T* resource_ = nullptr; // rraw pointer to the T object this handle owns.
    std::size_t index_ = static_cast<std::size_t>(-1); //  Set to size_t(-1) as a no slot owned

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
    // Alias
    using Handle = PoolHandle<T>;

    // Constructor

    explicit ResourcePool(
         std::size_t capacity,
         std::function<T()> factory,
         std::function<void(T&)> reset = {}) 
        : state_(std::make_shared<typename Handle::State>()) {
        if (capacity == 0) {throw std::invalid_argument(" capacity should be greater then 1");}  // A pool with 0 slots >> useless.

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

    // Destructor 
    ~ResourcePool() = default;

    // acquire() — blocking R2

    Handle acquire() {
        std::unique_lock<std::mutex> lock(state_->mutex);

        state_->cv.wait(lock, [this] {return !state_->available.empty();}); //R5

        const std::size_t index = state_->available.front();
        state_->available.pop(); 

        return Handle(state_, &state_->resources[index], index);
    }

    // acquire(timeout)  R3

    template <class Rep, class Period>
    std::optional<Handle> acquire(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(state_->mutex);

        bool got_one = state_->cv.wait_for(lock, timeout, [this] {
            return !state_->available.empty(); }); // R5

        if (!got_one)
            return std::nullopt;

        std::size_t index = state_->available.front();
        state_->available.pop();

        return Handle(state_, &state_->resources[index], index);
    }

private:
    // Data members
    std::shared_ptr<typename Handle::State> state_;
};
