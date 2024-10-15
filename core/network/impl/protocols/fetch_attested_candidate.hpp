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
#include "parachain/validator/statement_distribution/statement_distribution.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  class FetchAttestedCandidateProtocol final
      : public RequestResponseProtocolImpl<vstaging::AttestedCandidateRequest,
                                           vstaging::AttestedCandidateResponse,
                                           ScaleMessageReadWriter>,
        NonCopyable,
        NonMovable {
   public:
    FetchAttestedCandidateProtocol(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<
            parachain::statement_distribution::StatementDistribution>
            statement_distribution)
        : RequestResponseProtocolImpl<
              vstaging::AttestedCandidateRequest,
              vstaging::AttestedCandidateResponse,
              ScaleMessageReadWriter>{kFetchAttestedCandidateProtocolName,
                                      host,
                                      make_protocols(
                                          kFetchAttestedCandidateProtocol,
                                          genesis_hash,
                                          kProtocolPrefixPolkadot),
                                      log::createLogger(
                                          kFetchAttestedCandidateProtocolName,
                                          "req_attested_candidate_protocol")},
          statement_distribution_(std::move(statement_distribution)) {
      BOOST_ASSERT(statement_distribution_);
    }

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType request, std::shared_ptr<Stream> stream) override {
      base().logger()->info(
          "Fetching attested candidate request.(candidate={})",
          request.candidate_hash);
      statement_distribution_->OnFetchAttestedCandidateRequest(
          std::move(request), stream);
      return std::nullopt;
    }

    void onTxRequest(const RequestType &request) override {
      base().logger()->debug("Fetching attested candidate. (candidate={})",
                             request.candidate_hash);
    }

    inline static const auto kFetchAttestedCandidateProtocolName =
        "FetchAttestedCandidateProtocol"s;
    std::shared_ptr<parachain::statement_distribution::StatementDistribution>
        statement_distribution_;
  };

}  // namespace kagome::network
