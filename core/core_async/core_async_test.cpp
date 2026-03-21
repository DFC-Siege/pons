#include <catch2/catch_test_macros.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include "base_future.hpp"
#include "base_promise.hpp"
#include "semaphore_concept.hpp"

struct TestSemaphore {
        void give() {
                std::unique_lock lock(mutex);
                ready = true;
                cv.notify_one();
        }
        void give_from_isr() {
                give();
        }
        bool take() {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this] { return ready; });
                ready = false;
                return true;
        }
        bool take(uint32_t timeout_ms) {
                std::unique_lock lock(mutex);
                const auto result =
                    cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                [this] { return ready; });
                if (result)
                        ready = false;
                return result;
        }

      private:
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
};

static_assert(async::SemaphoreConcept<TestSemaphore>);

static bool wait_with_timeout(std::function<void()> fn, uint32_t timeout_ms) {
        std::atomic<bool> done = false;
        std::thread t([&] {
                fn();
                done = true;
        });
        t.detach();
        const auto start = std::chrono::steady_clock::now();
        while (!done) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count() > timeout_ms)
                        return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
}

TEST_CASE("promise set_value is received by future get") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        std::thread t([&] { promise.set_value(42); });
        int result = 0;
        REQUIRE(wait_with_timeout([&] { result = future->get(); }, 1000));
        REQUIRE(result == 42);
        t.join();
}

TEST_CASE("is_ready returns false before value is set") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        REQUIRE(!future->is_ready());
}

TEST_CASE("is_ready returns true after value is set") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        promise.set_value(1);
        REQUIRE(future->is_ready());
}

TEST_CASE("wait_for returns false on timeout") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        REQUIRE(!future->wait_for(10));
}

TEST_CASE("wait_for returns true when value is set") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        std::thread t([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                promise.set_value(1);
        });
        REQUIRE(future->wait_for(1000));
        t.join();
}

TEST_CASE("future works with string type") {
        async::BasePromise<std::string, TestSemaphore> promise;
        auto future = promise.get_future();
        std::thread t([&] { promise.set_value("hello"); });
        std::string result;
        REQUIRE(wait_with_timeout([&] { result = future->get(); }, 1000));
        REQUIRE(result == "hello");
        t.join();
}

TEST_CASE("wait_for then get returns value") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        promise.set_value(42);
        REQUIRE(future->wait_for(100));
        int result = 0;
        const auto completed =
            wait_with_timeout([&] { result = future->get(); }, 100);
        REQUIRE(completed);
        REQUIRE(result == 42);
}

TEST_CASE("is_ready then get returns value") {
        async::BasePromise<int, TestSemaphore> promise;
        auto future = promise.get_future();
        promise.set_value(42);
        REQUIRE(future->is_ready());
        int result = 0;
        REQUIRE(wait_with_timeout([&] { result = future->get(); }, 1000));
        REQUIRE(result == 42);
}
