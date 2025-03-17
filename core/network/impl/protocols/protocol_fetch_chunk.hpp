/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "blockchain/genesis_block_hash.hpp"
#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/peer_manager.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl;

  class FetchChunkProtocol
      : virtual public RequestResponseProtocol<FetchChunkRequest,
                                               FetchChunkResponse> {};

  class FetchChunkProtocolImpl final
      : public FetchChunkProtocol,
        public RequestResponseProtocolImpl<FetchChunkRequest,
                                           FetchChunkResponse,
                                           ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
    static constexpr std::chrono::seconds kRequestTimeout{1};

   public:
    FetchChunkProtocolImpl(RequestResponseInject inject,
                           const application::ChainSpec & /*chain_spec*/,
                           const blockchain::GenesisBlockHash &genesis_hash,
                           std::shared_ptr<parachain::ParachainStorage> pp,
                           std::shared_ptr<PeerManager> pm)
        : RequestResponseProtocolImpl<
            FetchChunkRequest,
            FetchChunkResponse,
            ScaleMessageReadWriter>{kFetchChunkProtocolName,
                                    std::move(inject),
                                    make_protocols(kFetchChunkProtocol,
                                                   genesis_hash,
                                                   kProtocolPrefixPolkadot),
                                    log::createLogger(kFetchChunkProtocolName,
                                                      "req_chunk_protocol"),
                                    kRequestTimeout},
          pp_{std::move(pp)},
          pm_{std::move(pm)} {
      BOOST_ASSERT(pp_);
      BOOST_ASSERT(pm_);
    }

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType request, std::shared_ptr<Stream> stream) override {
      SL_DEBUG(base_.logger(),
               "Fetching chunk request.(chunk={}, candidate={})",
               request.chunk_index,
               request.candidate);

      auto peer_id = [&] {
        auto res = stream->remotePeerId();
        BOOST_ASSERT(res.has_value());
        return res.value();
      }();
      auto &peer_state = [&]() -> PeerState & {
        auto res = pm_->getPeerState(peer_id);
        if (!res) {
          SL_TRACE(base_.logger(),
                   "No PeerState of peer {}. Default one has created",
                   peer_id);
          res = pm_->createDefaultPeerState(peer_id);
        }
        return res.value().get();
      }();
      peer_state.req_chunk_version = ReqChunkVersion::V2;

      SL_TRACE(
          base_.logger(),
          "ChunkRequest (v2) received from peer {}: candidate={}, chunk={}",
          peer_id,
          request.candidate,
          request.chunk_index);

      auto res = pp_->OnFetchChunkRequest(request);
      if (res.has_error()) {
        SL_ERROR(base_.logger(),
                 "Fetching chunk response failed.(error={})",
                 res.error());
        return res.as_failure();
      }

      if (auto _chunk = if_type<const network::Chunk>(res.value())) {
        SL_DEBUG(base_.logger(), "Fetching chunk response with data.");

        auto &chunk = _chunk.value().get();
        SL_TRACE(base_.logger(),
                 "ChunkResponse (v2) sent to peer {}: "
                 "chunk={}, data={}, proof=[{}]",
                 peer_id,
                 chunk.chunk_index,
                 chunk.data,
                 fmt::join(chunk.proof, ", "));
      } else {
        SL_DEBUG(base_.logger(), "Fetching chunk response empty.");

        SL_TRACE(base_.logger(),
                 "ChunkResponse (v2) sent to peer {}: empty",
                 peer_id);
      }

      return std::move(res);
    }

    void onTxRequest(const RequestType &request) override {
      SL_DEBUG(base_.logger(),
               "Fetching chunk candidate: {}, index: {}",
               request.candidate,
               request.chunk_index);
    }

    inline static const auto kFetchChunkProtocolName = "FetchChunkProtocol_v2"s;
    std::shared_ptr<parachain::ParachainStorage> pp_;
    std::shared_ptr<PeerManager> pm_;
  };

}  // namespace kagome::network
