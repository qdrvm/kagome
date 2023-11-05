/**
 * Copyright Quadrivium Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_FETCH_ATTESTED_CANDIDATE_HPP
#define KAGOME_PROTOCOL_FETCH_ATTESTED_CANDIDATE_HPP

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

  class FetchAttestedCandidateProtocol final
      : public RequestResponseProtocol<AttestedCandidateRequest,
                                       AttestedCandidateResponse,
                                       ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    FetchAttestedCandidateProtocol() = delete;
    ~FetchAttestedCandidateProtocol() override = default;

    FetchAttestedCandidateProtocol(libp2p::Host &host,
                       const application::ChainSpec &chain_spec,
                       const blockchain::GenesisBlockHash &genesis_hash,
                       std::shared_ptr<parachain::ParachainProcessorImpl> pp)
        : RequestResponseProtocol<
            AttestedCandidateRequest,
            AttestedCandidateResponse,
            ScaleMessageReadWriter>{kFetchAttestedCandidateProtocolName,
                                    host,
                                    make_protocols(kFetchAttestedCandidateProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(kFetchAttestedCandidateProtocolName,
                                                      "req_attested_candidate_protocol")},
          pp_{std::move(pp)} {
      BOOST_ASSERT(pp_);
    }

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType request, std::shared_ptr<Stream> /*stream*/) override {
      base().logger()->info("Fetching attested candidate request.(candidate={})",
                            request.candidate_hash);
      auto res = pp_->OnFetchAttestedCandidateRequest(std::move(request));
      if (res.has_error()) {
        base().logger()->error("Fetching attested candidate response failed.(error={})",
                               res.error().message());
      } else {
        base().logger()->trace("Fetching attested candidate response.");
      }
      return res;
    }

    void onTxRequest(const RequestType &request) override {
      base().logger()->debug("Fetching attested candidate. (candidate={})", request.candidate_hash);
    }

    inline static const auto kFetchAttestedCandidateProtocolName = "FetchAttestedCandidateProtocol"s;
    std::shared_ptr<parachain::ParachainProcessorImpl> pp_;
  };

}  // namespace kagome::network

#endif  // KAGOME_PROTOCOL_FETCH_ATTESTED_CANDIDATE_HPP
