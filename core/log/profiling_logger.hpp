/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
#define KAGOME_CORE_LOG_PROFILING_LOGGER_HPP

#include "log/logger.hpp"

namespace kagome::log {
  extern Logger profiling_logger;
}

#ifndef KAGOME_PROFILING

#include "clock/impl/clock_impl.hpp"

#define KAGOME_PROFILE_START(scope) \
  auto _profiling_start_##scope = ::kagome::clock::SteadyClockImpl{}.now();

#define KAGOME_PROFILE_END(scope)                                         \
  auto _profiling_end_##scope = ::kagome::clock::SteadyClockImpl{}.now(); \
  SL_DEBUG(::kagome::log::profiling_logger,                              \
           "{} took {} ms",                                               \
           #scope,                                                        \
           ::std::chrono::duration_cast<::std::chrono::milliseconds>(     \
               _profiling_end_##scope - _profiling_start_##scope)         \
               .count());

#else

#define KAGOME_PROFILE_START(scope)
#define KAGOME_PROFILE_END(scope)

#endif

#endif  // KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
