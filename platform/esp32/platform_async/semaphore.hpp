#pragma once

#include "esp32_semaphore.hpp"
#include "semaphore_concept.hpp"

namespace async {
using Semaphore = Esp32Semaphore;
static_assert(SemaphoreConcept<Semaphore>);
} // namespace async
