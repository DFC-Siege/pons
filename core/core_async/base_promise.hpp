#pragma once

#include <memory>

#include "base_future.hpp"
#include "i_promise.hpp"

namespace async {

template <typename T, SemaphoreConcept Sem>
class BasePromise : public IPromise<T> {
      public:
        BasePromise() : future(std::make_shared<BaseFuture<T, Sem>>()) {
        }

        void set_value(T val) override {
                future->set_value(std::move(val));
        }

        std::shared_ptr<IFuture<T>> get_future() override {
                return future;
        }

      private:
        std::shared_ptr<BaseFuture<T, Sem>> future;
};

} // namespace async
