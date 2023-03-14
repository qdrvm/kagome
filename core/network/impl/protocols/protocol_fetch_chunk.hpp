/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_FETCH_CHUNK_HPP
#define KAGOME_PROTOCOL_FETCH_CHUNK_HPP

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/impl/stream_engine.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct ReqPovProtocolImpl;

  class FetchChunkProtocol final
      : public RequestResponseProtocol<FetchChunkRequest,
                                       FetchChunkResponse,
                                       ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    FetchChunkProtocol() = delete;
    ~FetchChunkProtocol() override = default;

    FetchChunkProtocol(libp2p::Host &host,
                       application::ChainSpec const &chain_spec,
                       const primitives::BlockHash &genesis_hash,
                       std::shared_ptr<parachain::ParachainProcessorImpl> pp)
        : RequestResponseProtocol<
            FetchChunkRequest,
            FetchChunkResponse,
            ScaleMessageReadWriter>{kFetchChunkProtocolName,
                                    host,
                                    make_protocols(kFetchChunkProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(kFetchChunkProtocolName,
                                                      "req_chunk_protocol")},
          pp_{std::move(pp)} {
      BOOST_ASSERT(pp_);
    }

   private:
    outcome::result<ResponseType> onRxRequest(
        RequestType request, std::shared_ptr<Stream> /*stream*/) override {
      base().logger()->info("Fetching chunk request.(chunk={}, candidate={})",
                            request.chunk_index,
                            request.candidate);
      auto res = pp_->OnFetchChunkRequest(std::move(request));
      if (res.has_error()) {
        base().logger()->error("Fetching chunk response failed.(error={})",
                               res.error().message());
      } else {
        visit_in_place(
            res.value(),
            [&](network::Chunk const &r) {
              base().logger()->info("Fetching chunk response with data.");
            },
            [&](auto const &) {
              base().logger()->info("Fetching chunk response empty.");
            });
      }
      return res;
    }

    void onTxRequest(RequestType const &request) override {
      base().logger()->debug("Fetching chunk candidate: {}, index: {}",
                             request.candidate,
                             request.chunk_index);
    }

    const static inline auto kFetchChunkProtocolName = "FetchChunkProtocol"s;
    std::shared_ptr<parachain::ParachainProcessorImpl> pp_;
  };

}  // namespace kagome::network

#endif  // KAGOME_PROTOCOL_FETCH_CHUNK_HPP
