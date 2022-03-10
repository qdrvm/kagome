/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTUTIL_PREPARELOGGERS
#define TESTUTIL_PREPARELOGGERS

#include <log/logger.hpp>

#include <mutex>

#include <libp2p/log/configurator.hpp>

#include <log/configurator.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

namespace testutil {

  static std::once_flag initialized;

  // supposed to be called in SetUpTestCase
  void prepareLoggers(soralog::Level level = soralog::Level::INFO) {
    std::call_once(initialized, [] {
      auto testing_log_config = std::string(R"(
sinks:
  - name: console
    type: console
    capacity: 4
    latency: 0
groups:
  - name: main
    sink: console
    level: info
    is_fallback: true
    children:
      - name: testing
        level: trace
      - name: libp2p
        level: off
)");

      auto logging_system = std::make_shared<soralog::LoggingSystem>(
          std::make_shared<kagome::log::Configurator>(
              std::make_shared<libp2p::log::Configurator>(testing_log_config)));
      auto r = logging_system->configure();
      if (r.has_error) {
        throw std::runtime_error("Can't configure logger system: " + r.message);
      }

      kagome::log::setLoggingSystem(logging_system);
    });

    kagome::log::setLevelOfGroup(kagome::log::defaultGroupName, level);
  }
}  // namespace testutil

#endif  // TESTUTIL_PREPARELOGGERS
