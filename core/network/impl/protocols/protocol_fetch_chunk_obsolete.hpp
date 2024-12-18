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
#include "network/impl/protocols/request_response_protocol.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  class FetchChunkProtocolObsolete
      : virtual public RequestResponseProtocol<FetchChunkRequest,
                                               FetchChunkResponseObsolete> {};

  /// Implementation of first implementation of
  /// fetching chunk protocol aka 'req_chunk/1'
  ///
  /// In response index of systematic chunk is corresponding validator index.
  class FetchChunkProtocolObsoleteImpl final
      : public FetchChunkProtocolObsolete,
        public RequestResponseProtocolImpl<FetchChunkRequest,
                                           FetchChunkResponseObsolete,
                                           ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    FetchChunkProtocolObsoleteImpl(
        libp2p::Host &host,
        const application::ChainSpec & /*chain_spec*/,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<parachain::ParachainStorage> pp)
        : RequestResponseProtocolImpl<
            FetchChunkRequest,
            FetchChunkResponseObsolete,
            ScaleMessageReadWriter>{kFetchChunkProtocolName,
                                    host,
                                    make_protocols(kFetchChunkProtocolObsolete,
                                                   genesis_hash,
                                                   kProtocolPrefixPolkadot),
                                    log::createLogger(kFetchChunkProtocolName,
                                                      "req_chunk_protocol")},
          pp_{std::move(pp)} {
      BOOST_ASSERT(pp_);
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
      SL_TRACE(
          base_.logger(),
          "ChunkRequest (v1) received from peer {}: candidate={}, chunk={}",
          peer_id,
          request.candidate,
          request.chunk_index);

      auto res = pp_->OnFetchChunkRequestObsolete(request);
      if (res.has_error()) {
        SL_ERROR(base_.logger(),
                 "Fetching chunk response failed.(error={})",
                 res.error());
        return res.as_failure();
      }

      if (auto _chunk = if_type<const network::ChunkObsolete>(res.value())) {
        SL_DEBUG(base_.logger(), "Fetching chunk response with data.");

        auto &chunk = _chunk.value().get();
        SL_TRACE(base_.logger(),
                 "ChunkResponse (v1) sent to peer {}: "
                 "chunk={}, data={}, proof=[{}]",
                 peer_id,
                 chunk.data,
                 fmt::join(chunk.proof, ", "));
      } else {
        SL_DEBUG(base_.logger(), "Fetching chunk response empty.");

        SL_TRACE(base_.logger(),
                 "ChunkResponse (v1) sent to peer {}: empty",
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

    inline static const auto kFetchChunkProtocolName = "FetchChunkProtocol_v1"s;
    std::shared_ptr<parachain::ParachainStorage> pp_;
  };

}  // namespace kagome::network
