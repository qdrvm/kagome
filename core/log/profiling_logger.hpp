/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
#define KAGOME_CORE_LOG_PROFILING_LOGGER_HPP

#include "log/logger.hpp"

#include "clock/impl/clock_impl.hpp"

namespace kagome::log {

  extern Logger profiling_logger;

  struct ProfileScope {
    ProfileScope(std::string_view scope, log::Logger logger)
        : scope{scope}, logger{logger} {
      BOOST_ASSERT(logger != nullptr);
      start = ::kagome::clock::SteadyClockImpl{}.now();
    }

    ProfileScope(ProfileScope &&) = delete;
    ProfileScope(ProfileScope const &) = delete;
    ProfileScope &operator=(ProfileScope const &) = delete;
    ProfileScope &operator=(ProfileScope &&) = delete;

    ~ProfileScope() {
      if (!done) {
        end();
      }
    }

    void end() {
      done = true;
      auto end = ::kagome::clock::SteadyClockImpl{}.now();
      SL_INFO(
          logger,
          "{} took {} ms",
          scope,
          ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start)
              .count());
    }

   private:
    bool done = false;
    std::string_view scope{};
    kagome::clock::SteadyClock::TimePoint start;
    log::Logger logger;
  };
}  // namespace kagome::log

#ifdef KAGOME_PROFILING

#define KAGOME_PROFILE_START_L(logger, scope) \
  auto _profiling_scope_##scope = ::kagome::log::ProfileScope{#scope, logger};

#define KAGOME_PROFILE_END_L(logger, scope)                               \
  _profiling_scope_##scope .end();

#define KAGOME_PROFILE_START(scope) \
  KAGOME_PROFILE_START_L(::kagome::log::profiling_logger, scope)
#define KAGOME_PROFILE_END(scope) \
  KAGOME_PROFILE_END_L(::kagome::log::profiling_logger, scope)

#else

#define KAGOME_PROFILE_START(scope)
#define KAGOME_PROFILE_END(scope)

#define KAGOME_PROFILE_START_L(logger, scope)
#define KAGOME_PROFILE_END_L(logger, scope)

#endif

#endif  // KAGOME_CORE_LOG_PROFILING_LOGGER_HPP
