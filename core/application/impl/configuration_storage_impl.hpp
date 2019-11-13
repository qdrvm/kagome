/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
#define KAGOME_CONFIGURATION_STORAGE_IMPL_HPP

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "application/configuration_storage.hpp"
#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  class ConfigurationStorageImpl : public ConfigurationStorage {
   public:
    ConfigurationStorageImpl(KagomeConfig config);
    ~ConfigurationStorageImpl() override = default;

    const primitives::Block &getGenesis() const override;

   private:
    KagomeConfig config_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_IMPL_HPP
