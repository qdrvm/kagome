/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/genesis_block_hash.hpp"
#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/backing/store.hpp"

namespace kagome::network {

  class FetchAvailableDataProtocol
      : virtual public RequestResponseProtocol<FetchAvailableDataRequest,
                                               FetchAvailableDataResponse> {};

  class FetchAvailableDataProtocolImpl final
      : public FetchAvailableDataProtocol,
        public RequestResponseProtocolImpl<FetchAvailableDataRequest,
                                           FetchAvailableDataResponse,
                                           ScaleMessageReadWriter> {
   public:
    static constexpr const char *kName = "FetchAvailableDataProtocol";

    FetchAvailableDataProtocolImpl(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<parachain::AvailabilityStore> av_store)
        : RequestResponseProtocolImpl<
              FetchAvailableDataRequest,
              FetchAvailableDataResponse,
              ScaleMessageReadWriter>{kName,
                                      host,
                                      make_protocols(
                                          kFetchAvailableDataProtocol,
                                          genesis_hash,
                                          kProtocolPrefixPolkadot),
                                      log::createLogger(
                                          kName,
                                          "req_available_data_protocol")},
          av_store_{std::move(av_store)} {}

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType candidate_hash, std::shared_ptr<Stream>) override {
      SL_TRACE(base().logger(),
               "Fetch available data .(candidate hash={})",
               candidate_hash);

      if (auto r = av_store_->getPovAndData(candidate_hash)) {
        return std::move(*r);
      }
      return Empty{};
    }

    void onTxRequest(const RequestType &) override {}

    std::shared_ptr<parachain::AvailabilityStore> av_store_;
  };

  class StatementFetchingProtocol final
      : public RequestResponseProtocolImpl<FetchStatementRequest,
                                           FetchStatementResponse,
                                           ScaleMessageReadWriter> {
   public:
    static constexpr const char *kName = "FetchStatementProtocol";

    StatementFetchingProtocol(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<parachain::BackingStore> backing_store)
        : RequestResponseProtocolImpl<
              FetchStatementRequest,
              FetchStatementResponse,
              ScaleMessageReadWriter>{kName,
                                      host,
                                      make_protocols(kFetchStatementProtocol,
                                                     genesis_hash,
                                                     kProtocolPrefixPolkadot),
                                      log::createLogger(
                                          kName, "req_statement_protocol")},
          backing_store_{std::move(backing_store)} {}

   private:
    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType req, std::shared_ptr<Stream>) override {
      SL_TRACE(base().logger(),
               "Statement fetch request received.(relay parent={}, candidate "
               "hash={})",
               req.relay_parent,
               req.candidate_hash);

      if (auto res = backing_store_->getCadidateInfo(req.relay_parent,
                                                     req.candidate_hash)) {
        return res->get().candidate;
      }

      base().logger()->error("No fetch statement response.");
      return {{ProtocolError::NO_RESPONSE}};
    }

    void onTxRequest(const RequestType &) override {}

    std::shared_ptr<parachain::BackingStore> backing_store_;
  };
}  // namespace kagome::network
