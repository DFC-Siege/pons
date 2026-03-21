#pragma once
#include <memory>

#include "i_future.hpp"

namespace async {

template <typename T> class IPromise {
      public:
        virtual ~IPromise() = default;
        virtual void set_value(T val) = 0;
        virtual std::shared_ptr<IFuture<T>> get_future() = 0;
};

} // namespace async
