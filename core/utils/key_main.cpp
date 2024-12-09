/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "application/impl/app_configuration_impl.hpp"
#include "injector/application_injector.hpp"
#include "key/key.hpp"

namespace kagome {
  int key_main(int argc, const char **argv) {
    if (argc == 2) {
      if (std::string(argv[1]) == "--generate-node-key") {
        injector::KagomeNodeInjector injector{
            std::make_shared<application::AppConfigurationImpl>()};
        auto key = injector.injectKey();
        auto res = key->run();
      } else if (std::string(argv[1]) == "--help") {
        std::cerr << "Usage: " << argv[0] << " --generate-node-key"
                  << "\nGenerates a node key and prints the peer ID to stderr "
                     "and the secret key to stdout."
                  << std::endl;
      } else {
        std::cerr << "Unknown command: " << argv[1] << std::endl;
        std::cerr << "Usage: " << argv[0] << " --generate-node-key"
                  << std::endl;
        return 3;
      }
    } else {
      std::cerr << "Usage: " << argv[0] << " --generate-node-key" << std::endl;
      return 1;
    }
    return 0;
  }

}  // namespace kagome
