#include <gtest/gtest.h>
#include "resource_pool.hpp"

// Helper type used across tests.
struct Resource {
    int value = 0;
};

// ── Test 1: Basic acquire and release ────────────────────────────────────────
//
// A pool with capacity 1 should hand out a handle on acquire() and make the
// resource available again after the handle is destroyed.

TEST(ResourcePoolTest, BasicAcquireAndRelease) {
    ResourcePool<Resource> pool(
        1,
        [] { return Resource{}; }
    );

    {
        auto h = pool.acquire();
        h->value = 10;
        EXPECT_EQ(h->value, 10);
    }

    auto h2 = pool.acquire();
    EXPECT_EQ(h2->value, 0);
}

// ── Test 2: Pool exhaustion — blocked acquire unblocks after release ──────────
//
// When all resources are acquired, a second acquire() on a separate thread
// must block until the first handle is released.

TEST(ResourcePoolTest, BlocksWhenExhaustedAndUnblocksOnRelease) {
    ResourcePool<Resource> pool(1, [] { return Resource{}; });

    auto h1 = pool.acquire();
    std::atomic<bool> acquired{ false }; 

    std::thread t([&] {
        auto h2 = pool.acquire();
        acquired = true;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(acquired.load());

    h1.~PoolHandle();  // release h1, now t should be unblocked
    t.join();

    EXPECT_TRUE(acquired.load());
}

// ── Test 3: Timed acquire returns empty optional on timeout ───────────────────
//
// acquire(timeout) must return std::nullopt if no resource becomes available
// within the given duration.

TEST(ResourcePoolTest, TimedAcquireReturnsNulloptOnTimeout) {
    FAIL() << "Not implemented";
}

// ── Test 4: RAII — resource returned automatically on scope exit ──────────────
//
// Destroying the handle (scope exit, not explicit call) must return the
// resource to the pool and allow a subsequent acquire() to succeed.

TEST(ResourcePoolTest, HandleReleasesOnScopeExit) {
    FAIL() << "Not implemented";
}

// ── Test 5: Concurrent stress test ───────────────────────────────────────────
//
// At least 4 threads each perform multiple acquire/use/release cycles
// concurrently. The test must complete without deadlock, crash, or data race
// (run with ThreadSanitizer to verify).

TEST(ResourcePoolTest, ConcurrentAcquireRelease) {
    FAIL() << "Not implemented";
}

// ── Test 6: Reset function called on every release ───────────────────────────
//
// If a reset function is provided, it must be called exactly once each time
// a resource is returned to the pool, before the resource is made available
// for the next acquire().

TEST(ResourcePoolTest, ResetFunctionCalledOnRelease) {
    FAIL() << "Not implemented";
}

// ── Test 7: Moved-from handle is safely destructible ─────────────────────────
//
// Moving a handle must leave the source in a valid state. Destroying the
// moved-from handle must not crash and must not return the resource a second
// time.

TEST(ResourcePoolTest, MovedFromHandleIsSafeToDestroy) {
    FAIL() << "Not implemented";
}
