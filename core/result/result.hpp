#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <string_view>

namespace result {
template <typename T> class Result {
      public:
        Result(bool fail, std::optional<std::string> err, std::optional<T> val)
            : fail(fail), err(std::move(err)), val(std::move(val)) {
        }
        bool failed() const {
                return fail;
        }
        std::string_view error() const {
                if (err.has_value())
                        return *err;
                return "";
        }
        const T &value() const & {
                assert(!fail && "called value() on a failed result");
                return val.value();
        }
        T &&value() && {
                assert(!fail && "called value() on a failed result");
                return std::move(val.value());
        }

      private:
        bool fail;
        std::optional<std::string> err;
        std::optional<T> val;
};

template <typename T> class Result<T &> {
      public:
        Result(bool fail, std::optional<std::string> err, T *val)
            : fail(fail), err(std::move(err)), val(val) {
        }
        bool failed() const {
                return fail;
        }
        std::string_view error() const {
                if (err.has_value())
                        return *err;
                return "";
        }
        const T &value() const {
                assert(val != nullptr && "called value() on a failed result");
                return *val;
        }

      private:
        bool fail;
        std::optional<std::string> err;
        T *val;
};

struct Error {
        std::string message;
        template <typename T> operator Result<T>() const & {
                return {true, message, std::nullopt};
        }
        template <typename T> operator Result<T>() && {
                return {true, std::move(message), std::nullopt};
        }
        template <typename T> operator Result<T &>() const & {
                return {true, message, nullptr};
        }
        template <typename T> operator Result<T &>() && {
                return {true, std::move(message), nullptr};
        }
};

using Try = Result<bool>;

inline Error err(std::string_view message) {
        return {std::string(message)};
}
inline Result<bool> ok() {
        return {false, std::nullopt, true};
}
template <typename T> Result<T> ok(T value) {
        return {false, std::nullopt, std::move(value)};
}
template <typename T> Result<T &> ok_ref(T &value) {
        return {false, std::nullopt, &value};
}

#define TRY(expr)                                                              \
        ({                                                                     \
                auto _result = (expr);                                         \
                if (_result.failed())                                          \
                        return result::err(_result.error());                   \
                std::move(_result).value();                                    \
        })
} // namespace result
