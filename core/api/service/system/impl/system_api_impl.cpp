/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/impl/system_api_impl.hpp"

#include <jsonrpc-lean/request.h>

namespace kagome::api {

  SystemApiImpl::SystemApiImpl(
      std::shared_ptr<application::ConfigurationStorage> config,
      std::shared_ptr<consensus::Babe> babe,
      std::shared_ptr<network::Gossiper> gossiper)
      : config_(std::move(config)),
        babe_(std::move(babe)),
        gossiper_(std::move(gossiper)) {
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(babe_ != nullptr);
    BOOST_ASSERT(gossiper_ != nullptr);
  }

  std::shared_ptr<application::ConfigurationStorage> SystemApiImpl::getConfig()
      const {
    return config_;
  }

  std::shared_ptr<consensus::Babe> SystemApiImpl::getBabe() const {
    return babe_;
  }

  std::shared_ptr<network::Gossiper> SystemApiImpl::getGossiper() const {
    return gossiper_;
  }

}  // namespace kagome::api
