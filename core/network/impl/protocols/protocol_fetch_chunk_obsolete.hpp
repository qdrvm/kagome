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
#include "network/impl/stream_engine.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl;

  /// Implementation of first implementation of
  /// fetching chunk protocol aka 'req_chunk/1'
  ///
  /// In response index of systematic chunk is corresponding validator index.
  class FetchChunkProtocolObsolete final
      : public RequestResponseProtocolImpl<FetchChunkRequest,
                                           FetchChunkResponseObsolete,
                                           ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    FetchChunkProtocolObsolete(
        libp2p::Host &host,
        const application::ChainSpec & /*chain_spec*/,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<parachain::ParachainProcessorImpl> pp)
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
        RequestType request, std::shared_ptr<Stream> /*stream*/) override {
      base().logger()->info("Fetching chunk request.(chunk={}, candidate={})",
                            request.chunk_index,
                            request.candidate);
      auto res = pp_->OnFetchChunkRequest(std::move(request));
      if (res.has_error()) {
        base().logger()->error("Fetching chunk response failed.(error={})",
                               res.error());
        return res.as_failure();
      }

      if (auto chunk = if_type<const network::Chunk>(res.value())) {
        base().logger()->info("Fetching chunk response with data.");
        return outcome::success(network::ChunkObsolete{
            .data = std::move(chunk.value().get().data),
            .proof = std::move(chunk.value().get().proof),
        });
      }

      base().logger()->info("Fetching chunk response empty.");
      return outcome::success(network::Empty{});
    }

    void onTxRequest(const RequestType &request) override {
      base().logger()->debug("Fetching chunk candidate: {}, index: {}",
                             request.candidate,
                             request.chunk_index);
    }

    inline static const auto kFetchChunkProtocolName = "FetchChunkProtocol_v1"s;
    std::shared_ptr<parachain::ParachainProcessorImpl> pp_;
  };

}  // namespace kagome::network
