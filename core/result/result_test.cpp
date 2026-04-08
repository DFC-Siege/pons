#include <catch2/catch_test_macros.hpp>

#include "result.hpp"

TEST_CASE("ok() produces non-failed result") {
        const auto r = result::ok();
        REQUIRE(!r.failed());
        REQUIRE(r.value() == true);
}

TEST_CASE("ok(value) produces non-failed result with value") {
        const auto r = result::ok(42);
        REQUIRE(!r.failed());
        REQUIRE(r.value() == 42);
}

TEST_CASE("err() produces failed result") {
        const result::Result<int> r = result::err("something went wrong");
        REQUIRE(r.failed());
        REQUIRE(r.error() == "something went wrong");
}


TEST_CASE("ok_ref() produces non-failed result with reference") {
        int val = 42;
        const auto r = result::ok_ref(val);
        REQUIRE(!r.failed());
        REQUIRE(r.value() == 42);
}

TEST_CASE("err() converts to Result<T&>") {
        const result::Result<int &> r = result::err("ref error");
        REQUIRE(r.failed());
        REQUIRE(r.error() == "ref error");
}

TEST_CASE("error message is preserved") {
        const result::Result<std::string> r = result::err("specific error");
        REQUIRE(r.error() == "specific error");
}

TEST_CASE("value can be moved out of rvalue result") {
        auto r = result::ok(std::string("hello"));
        const auto val = std::move(r).value();
        REQUIRE(val == "hello");
}

static result::Result<int> try_double(result::Result<int> input) {
        const auto val = TRY(input);
        return result::ok(val * 2);
}

TEST_CASE("TRY macro unwraps successful result") {
        const auto r = try_double(result::ok(21));
        REQUIRE(!r.failed());
        REQUIRE(r.value() == 42);
}

TEST_CASE("TRY macro propagates error") {
        const auto r = try_double(result::err("broken"));
        REQUIRE(r.failed());
        REQUIRE(r.error() == "broken");
}

TEST_CASE("Error implicitly converts to Result<T>") {
        result::Result<int> r = result::err("fail");
        REQUIRE(r.failed());
        REQUIRE(r.error() == "fail");
}

TEST_CASE("Error implicitly converts to Result<T&>") {
        result::Result<int &> r = result::err("ref fail");
        REQUIRE(r.failed());
        REQUIRE(r.error() == "ref fail");
}
