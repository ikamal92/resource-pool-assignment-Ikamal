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
    ResourcePool<Resource> pool(1, [] { return Resource{}; });
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
    ResourcePool<Resource> pool(1, [] { return Resource{}; });

    auto h1 = pool.acquire();
    auto result = pool.acquire(std::chrono::milliseconds(100));

    EXPECT_FALSE(result.has_value());
}

// ── Test 4: RAII — resource returned automatically on scope exit ──────────────
//
// Destroying the handle (scope exit, not explicit call) must return the
// resource to the pool and allow a subsequent acquire() to succeed.
// Already Covered by Test 1
TEST(ResourcePoolTest, HandleReleasesOnScopeExit) {
    ResourcePool<Resource> pool(1, [] { return Resource{}; });
    {
        auto h = pool.acquire();
        h->value = 10;
        EXPECT_EQ(h->value, 10);
    }

    auto h2 = pool.acquire();
    EXPECT_EQ(h2->value, 0);
}

// ── Test 5: Concurrent stress test ───────────────────────────────────────────
//
// At least 4 threads each perform multiple acquire/use/release cycles
// concurrently. The test must complete without deadlock, crash, or data race
// (run with ThreadSanitizer to verify).

TEST(ResourcePoolTest, ConcurrentAcquireRelease) {
    ResourcePool<Resource> pool(4, [] { return Resource{}; });
    std::atomic<int> counter{0};
    auto worker = [&] {
        for (int i = 0; i < 1000; i++) {
            auto h = pool.acquire();  // acquire
            h->value++;               // use
            counter++;                // increase counter for testing 
        }                             // Release 
        };
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);
    t1.join(); t2.join(); t3.join(); t4.join();
    EXPECT_EQ(counter.load(), 4000);
}

// ── Test 6: Reset function called on every release ───────────────────────────
//
// If a reset function is provided, it must be called exactly once each time
// a resource is returned to the pool, before the resource is made available
// for the next acquire().

TEST(ResourcePoolTest, ResetFunctionCalledOnRelease) {
    std::atomic<int> reset_count = 0;

    ResourcePool<Resource> pool(
        2,[] { return Resource{}; },
        [&](Resource& r) 
        {
            r.value = 0;
            reset_count++;
        }
    );

    {
        auto h1 = pool.acquire();
        h1->value = 99;
    }

    {
        auto h2 = pool.acquire();
        EXPECT_EQ(h2->value, 0);
    }

    EXPECT_EQ(reset_count.load(), 4); // as the reset func runs twice at factory construction, also twice when the two resources released so the final counter is 4 
}

// ── Test 7: Moved-from handle is safely destructible ─────────────────────────
//
// Moving a handle must leave the source in a valid state. Destroying the
// moved-from handle must not crash and must not return the resource a second
// time.

TEST(ResourcePoolTest, MovedFromHandleIsSafeToDestroy) {
    ResourcePool<Resource> pool(1,[] { return Resource{};});

    auto h1 = pool.acquire();
    auto h2 = std::move(h1);

}
