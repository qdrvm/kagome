/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/warp/cache.hpp"

namespace kagome::network {

  using WarpRequest = primitives::BlockHash;
  using WarpResponse = WarpSyncProof;

  class WarpProtocol
      : virtual public RequestResponseProtocol<WarpRequest, WarpResponse> {};

  class WarpProtocolImpl
      : public WarpProtocol,
        public RequestResponseProtocolImpl<WarpRequest,
                                           WarpResponse,
                                           ScaleMessageReadWriter> {
    static constexpr auto kName = "WarpProtocol";
    static constexpr std::chrono::seconds kRequestTimeout{10};

   public:
    WarpProtocolImpl(RequestResponseInject inject,
                     const application::ChainSpec &chain_spec,
                     const blockchain::GenesisBlockHash &genesis,
                     std::shared_ptr<WarpSyncCache> cache)
        : RequestResponseProtocolImpl(
              kName,
              std::move(inject),
              make_protocols(kWarpProtocol, genesis, chain_spec),
              log::createLogger(kName, "warp_sync_protocol"),
              kRequestTimeout),
          cache_{std::move(cache)} {}

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType after_hash, std::shared_ptr<Stream>) override {
      return cache_->getProof(after_hash);
    }

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<WarpSyncCache> cache_;
  };
}  // namespace kagome::network
