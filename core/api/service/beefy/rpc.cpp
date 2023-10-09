/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/beefy/rpc.hpp"

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/jrpc_fn.hpp"
#include "blockchain/block_header_repository.hpp"
#include "network/beefy/beefy.hpp"

namespace kagome::api {
  BeefyRpc::BeefyRpc(
      std::shared_ptr<JRpcServer> server,
      LazySPtr<network::Beefy> beefy,
      LazySPtr<blockchain::BlockHeaderRepository> block_header_repository)
      : server_{std::move(server)},
        beefy_{std::move(beefy)},
        block_header_repository_{std::move(block_header_repository)} {}

  void BeefyRpc::registerHandlers() {
    server_->registerHandler(
        "beefy_getFinalizedHead",
        jrpcFn(this, [](std::shared_ptr<BeefyRpc> self) {
          return self->block_header_repository_.get()->getHashByNumber(
              self->beefy_.get()->finalized());
        }));
  }
}  // namespace kagome::api
