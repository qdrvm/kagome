/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LOG_CONFIGURATOR
#define KAGOME_LOG_CONFIGURATOR

#include <boost/filesystem.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

namespace kagome::log {

  class Configurator : public soralog::ConfiguratorFromYAML {
    using PrevConfigurator = soralog::Configurator;

   public:
    Configurator(std::shared_ptr<PrevConfigurator> previous);

    explicit Configurator(std::shared_ptr<PrevConfigurator> previous,
                          std::string config);

    explicit Configurator(std::shared_ptr<PrevConfigurator> previous,
                          boost::filesystem::path path);
  };

}  // namespace kagome::log

#endif  // KAGOME_LOG_CONFIGURATOR
