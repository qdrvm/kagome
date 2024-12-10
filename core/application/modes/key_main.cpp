/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "application/impl/app_configuration_impl.hpp"
#include "injector/application_injector.hpp"
#include "key.hpp"

namespace kagome {
  int key_main(int argc, const char **argv) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const std::string_view key_command = argv[0];
    if (argc == 2) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      const std::string_view command = argv[1];
      if (command == "--generate-node-key") {
        auto injector = std::make_unique<injector::KagomeNodeInjector>(
            std::make_shared<application::AppConfigurationImpl>());
        auto key = injector->injectKey();
        if (auto res = key->run(); not res) {
          std::cerr << "Error: " << res.error().message() << "\n";
          return 2;
        }
      } else if (command == "--help") {
        std::cerr << "Usage: " << key_command << " --generate-node-key"
                  << "\nGenerates a node key and prints the peer ID to stderr "
                     "and the secret key to stdout.\n";
      } else {
        std::cerr << "Unknown command: " << command << "\n";
        std::cerr << "Usage: " << key_command << " --generate-node-key\n";
        return 3;
      }
    } else {
      std::cerr << "Usage: " << key_command << " --generate-node-key\n";
      return 1;
    }
    return 0;
  }

}  // namespace kagome
