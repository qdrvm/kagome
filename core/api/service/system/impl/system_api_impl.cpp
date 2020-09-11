/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/impl/system_api_impl.hpp"

#include <jsonrpc-lean/request.h>

namespace kagome::api {

  SystemApiImpl::SystemApiImpl(
      std::shared_ptr<application::ConfigurationStorage> config)
      : config_(std::move(config)) {
    BOOST_ASSERT(config_ != nullptr);
  }

  std::shared_ptr<application::ConfigurationStorage> SystemApiImpl::getConfig()
      const {
    if (not config_) {
      throw jsonrpc::InternalErrorFault(
          "Internal error. Configuration storage not initialized.");
    }
    return config_;
  }

}  // namespace kagome::api
