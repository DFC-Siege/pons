#pragma once

#include "posix_semaphore.hpp"
#include "semaphore_concept.hpp"

namespace async {
using Semaphore = PosixSemaphore;
static_assert(SemaphoreConcept<Semaphore>);
} // namespace async
