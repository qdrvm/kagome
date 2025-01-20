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

#include <random>

namespace kagome::network {

  using WarpRequest = primitives::BlockHash;
  using WarpResponse = WarpSyncProof;

  class WarpProtocol
      : virtual public RequestResponseProtocol<WarpRequest, WarpResponse> {
   public:
    using Cb = std::function<void(outcome::result<ResponseType>)>;
    virtual void random(WarpRequest req, Cb cb) = 0;
  };

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

    void random(WarpRequest req, Cb cb) override {
      std::vector<PeerId> peers;
      auto &protocols1 = base().protocolIds();
      std::set protocols2(protocols1.begin(), protocols1.end());
      auto &protocol_repo =
          base().host().getPeerRepository().getProtocolRepository();
      auto &conns = base().host().getNetwork().getConnectionManager();
      for (auto &peer : protocol_repo.getPeers()) {
        if (not conns.getBestConnectionForPeer(peer)) {
          continue;
        }
        auto common = protocol_repo.supportsProtocols(peer, protocols2);
        if (not common) {
          continue;
        }
        if (common.value().empty()) {
          continue;
        }
        peers.emplace_back(peer);
      }
      if (peers.empty()) {
        for (auto &conn : conns.getConnections()) {
          if (auto peer = conn->remotePeer()) {
            peers.emplace_back(peer.value());
          }
        }
      }
      if (peers.empty()) {
        return;
      }
      auto peer = peers[random_() % peers.size()];
      doRequest(peer, req, std::move(cb));
    }

   private:
    std::shared_ptr<WarpSyncCache> cache_;
    std::default_random_engine random_;
  };
}  // namespace kagome::network
