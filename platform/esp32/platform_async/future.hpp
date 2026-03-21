#pragma once

#include "base_future.hpp"
#include "semaphore.hpp"

namespace async {
template <typename T> using Future = BaseFuture<T, Semaphore>;
}
