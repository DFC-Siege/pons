#pragma once
#include <cassert>
#include <optional>
#include <string_view>

namespace result {

template <typename T> class Result {
      public:
        Result() : fail(true) {
        }

        Result(bool fail, std::string_view err, std::optional<T> val)
            : fail(fail), err(err), val(std::move(val)) {
        }

        bool failed() const {
                return fail;
        }
        std::string_view error() const {
                return err;
        }

        const T &value() const {
                assert(!fail && "called value() on a failed result");
                return val.value();
        }

      private:
        bool fail;
        std::string_view err;
        std::optional<T> val;
};

template <typename T> class Result<T &> {
      public:
        Result(bool fail, std::string_view err, T *val)
            : fail(fail), err(err), val(val) {
        }

        bool failed() const {
                return fail;
        }
        std::string_view error() const {
                return err;
        }

        const T &value() const {
                assert(val != nullptr && "called value() on a failed result");
                return *val;
        }

      private:
        bool fail;
        std::string_view err;
        T *val;
};

struct Error {
        std::string_view message;

        template <typename T> operator Result<T>() const {
                return {true, message, std::nullopt};
        }

        template <typename T> operator Result<T &>() const {
                return {true, message, nullptr};
        }
};

inline Error err(std::string_view message) {
        return {message};
}

inline Result<bool> ok() {
        return {false, "", true};
}

template <typename T> Result<T> ok(T value) {
        return {false, "", std::move(value)};
}

template <typename T> Result<T &> ok_ref(T &value) {
        return {false, "", &value};
}

} // namespace result
