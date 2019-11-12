/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/configuration_storage_impl.hpp"
#include "common/hexutil.hpp"

namespace kagome::application {

  ConfigurationStorageImpl::ConfigurationStorageImpl(KagomeConfig config)
      : config_{std::move(config)} {}

  primitives::Block ConfigurationStorageImpl::getGenesis() const {
    return config_.genesis;
  }

}  // namespace kagome::application
