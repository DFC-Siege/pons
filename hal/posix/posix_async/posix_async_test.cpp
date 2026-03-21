#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "posix_semaphore.hpp"

TEST_CASE("take blocks until give is called") {
        async::PosixSemaphore sem;
        bool done = false;
        std::thread t([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                done = true;
                sem.give();
        });
        sem.take();
        REQUIRE(done);
        t.join();
}

TEST_CASE("take returns true when signaled") {
        async::PosixSemaphore sem;
        std::thread t([&] { sem.give(); });
        REQUIRE(sem.take());
        t.join();
}

TEST_CASE("wait_for returns false on timeout") {
        async::PosixSemaphore sem;
        REQUIRE(!sem.take(10));
}

TEST_CASE("wait_for returns true when signaled within timeout") {
        async::PosixSemaphore sem;
        std::thread t([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                sem.give();
        });
        REQUIRE(sem.take(1000));
        t.join();
}

TEST_CASE("give_from_isr behaves like give") {
        async::PosixSemaphore sem;
        std::thread t([&] { sem.give_from_isr(); });
        REQUIRE(sem.take(1000));
        t.join();
}

TEST_CASE("multiple gives can be consumed sequentially") {
        async::PosixSemaphore sem;
        sem.give();
        sem.give();
        REQUIRE(sem.take(0));
        REQUIRE(sem.take(0));
}
