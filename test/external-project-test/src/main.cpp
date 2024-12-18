/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <application/impl/app_configuration_impl.hpp>
#include <injector/application_injector.hpp>
#include <libp2p/common/final_action.hpp>
#include <libp2p/log/configurator.hpp>
#include <log/configurator.hpp>
#include <log/logger.hpp>

int main() {
  libp2p::common::FinalAction flush_std_streams_at_exit([] {
    std::cout.flush();
    std::cerr.flush();
  });

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<kagome::log::Configurator>(
          std::make_shared<libp2p::log::Configurator>()));
  auto res = logging_system->configure();
  if (not res.message.empty()) {
    (res.has_error ? std::cerr : std::cout) << res.message << '\n';
  }
  if (res.has_error) {
    exit(EXIT_FAILURE);
  }
  kagome::log::setLoggingSystem(logging_system);

  auto injector = std::make_shared<kagome::injector::KagomeNodeInjector>(
      std::make_shared<kagome::application::AppConfigurationImpl>());
  [[maybe_unused]] auto executor = injector->injectExecutor();
  return 0;
}
