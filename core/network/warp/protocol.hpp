/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_WARP_PROTOCOL_HPP
#define KAGOME_NETWORK_WARP_PROTOCOL_HPP

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/warp/cache.hpp"

namespace kagome::network {
  class WarpProtocol : public RequestResponseProtocol<primitives::BlockHash,
                                                      WarpSyncProof,
                                                      ScaleMessageReadWriter> {
    static constexpr auto kName = "WarpProtocol";

   public:
    WarpProtocol(libp2p::Host &host,
                 const application::ChainSpec &chain_spec,
                 const primitives::GenesisBlockHeader &genesis,
                 std::shared_ptr<WarpSyncCache> cache)
        : RequestResponseProtocolType{
            kName,
            host,
            make_protocols(kWarpProtocol, genesis, chain_spec),
            log::createLogger(kName, "warp_sync_protocol"),
        },
        cache_{std::move(cache)} {}

    outcome::result<ResponseType> onRxRequest(
        RequestType after_hash, std::shared_ptr<Stream>) override {
      return cache_->getProof(after_hash);
    }

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<WarpSyncCache> cache_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_WARP_PROTOCOL_HPP
