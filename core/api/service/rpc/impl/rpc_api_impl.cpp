/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/rpc/impl/rpc_api_impl.hpp"

#include <utility>

namespace kagome::api {

  RpcApiImpl::RpcApiImpl(std::shared_ptr<JRpcServer> server)
      : server_{std::move(server)} {
    BOOST_ASSERT(server_ != nullptr);
  }

  outcome::result<std::vector<std::string>> RpcApiImpl::methods() const {
    return server_->getHandlerNames();
  }

}  // namespace kagome::api
