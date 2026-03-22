#pragma once

#include "rp2040_semaphore.hpp"
#include "semaphore_concept.hpp"

namespace async {
using Semaphore = RP2040Semaphore;
static_assert(SemaphoreConcept<Semaphore>);
} // namespace async
