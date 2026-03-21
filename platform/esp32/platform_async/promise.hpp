#pragma once

#include "base_promise.hpp"
#include "semaphore.hpp"

namespace async {
template <typename T> using Promise = BasePromise<T, Semaphore>;
}
