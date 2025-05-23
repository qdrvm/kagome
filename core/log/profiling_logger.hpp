/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

#include "clock/impl/clock_impl.hpp"

namespace kagome::log {

  Logger profiling_logger();

  struct ProfileScope {
    using Clock = ::kagome::clock::SteadyClockImpl;

    explicit ProfileScope(std::string_view scope,
                          log::Logger logger = profiling_logger())
        : scope{scope}, logger{std::move(logger)} {
      BOOST_ASSERT(logger != nullptr);
      start = Clock{}.now();
    }

    ProfileScope(ProfileScope &&) = delete;
    ProfileScope(const ProfileScope &) = delete;
    ProfileScope &operator=(const ProfileScope &) = delete;
    ProfileScope &operator=(ProfileScope &&) = delete;

    ~ProfileScope() {
      if (!done) {
        end();
      }
    }

    void end() {
      done = true;
      auto end = Clock{}.now();
      SL_DEBUG(
          logger,
          "{} took {} ms",
          scope,
          ::std::chrono::duration_cast<::std::chrono::milliseconds>(end - start)
              .count());
    }

   private:
    bool done = false;
    std::string_view scope;
    Clock::TimePoint start;
    log::Logger logger;
  };
}  // namespace kagome::log

#ifdef KAGOME_PROFILING

#define KAGOME_PROFILE_START_L(logger, scope) \
  auto _profiling_scope_##scope = ::kagome::log::ProfileScope{#scope, logger};

#define KAGOME_PROFILE_START(scope) \
  KAGOME_PROFILE_START_L(::kagome::log::profiling_logger(), scope)
#define KAGOME_PROFILE_END(scope) _profiling_scope_##scope.end();

#else

#define KAGOME_PROFILE_START(scope)
#define KAGOME_PROFILE_END(scope)

#define KAGOME_PROFILE_START_L(logger, scope)

#endif
