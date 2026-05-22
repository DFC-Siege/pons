#pragma once

#include "i_logger.hpp"
#include "logger.hpp"

namespace lg {

#define PONS_LOG_DEFINE_LEVEL(NS, METHOD, FN, LEVEL)                           \
        namespace NS {                                                         \
        template <typename... Args>                                            \
        inline void FN(logging::Tag tag, std::format_string<Args...> fmt,      \
                       Args &&...args) {                                       \
                logging::logger().METHOD(logging::LogLevel::LEVEL, tag, fmt,   \
                                         std::forward<Args>(args)...);         \
        }                                                                      \
        template <typename... Args>                                            \
        inline void FN(std::format_string<Args...> fmt, Args &&...args) {      \
                logging::logger().METHOD(logging::LogLevel::LEVEL, fmt,        \
                                         std::forward<Args>(args)...);         \
        }                                                                      \
        }

#define PONS_LOG_DEFINE_NS(NS, METHOD)                                         \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, verbose, Verbose)                    \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, debug, Debug)                        \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, info, Info)                          \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, warn, Warning)                       \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, error, Error)                        \
        PONS_LOG_DEFINE_LEVEL(NS, METHOD, fatal, Fatal)

PONS_LOG_DEFINE_NS(print, print_fmt)
PONS_LOG_DEFINE_NS(println, println_fmt)

#undef PONS_LOG_DEFINE_NS
#undef PONS_LOG_DEFINE_LEVEL

} // namespace lg
