/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
#define KAGOME_CORE_LOG_PROFILING_LOGGER_HPP

#include "log/logger.hpp"

namespace kagome::log {
  extern Logger _profiling_logger;
}


#ifndef NDEBUG  // TODO(Harrm): maybe add a separate compile-time constant

#include "clock/impl/clock_impl.hpp"

#define SL_PROFILE_START(scope) \
auto _profiling_start_##scope = ::kagome::clock::SteadyClockImpl{}.now();

#define SL_PROFILE_END(scope)                                             \
auto _profiling_end_##scope = ::kagome::clock::SteadyClockImpl{}.now(); \
SL_DEBUG(::kagome::log::_profiling_logger,                              \
"{} took {} ms",                                               \
           #scope,                                                        \
           ::std::chrono::duration_cast<::std::chrono::milliseconds>(     \
           _profiling_end_##scope - _profiling_start_##scope)         \
           .count());

#else

#define SL_PROFILE_START(scope)
#define SL_PROFILE_END(scope, msg, ...)

#endif


#endif  // KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
