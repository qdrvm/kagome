/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_PROTOCOLS_PROTOCOL_FETCH_AVAILABLE_DATA_HPP
#define KAGOME_NETWORK_IMPL_PROTOCOLS_PROTOCOL_FETCH_AVAILABLE_DATA_HPP

#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/backing/store.hpp"

namespace kagome::network {
  class FetchAvailableDataProtocol final
      : public RequestResponseProtocol<FetchAvailableDataRequest,
                                       FetchAvailableDataResponse,
                                       ScaleMessageReadWriter> {
   public:
    static constexpr const char *kName = "FetchAvailableDataProtocol";

    FetchAvailableDataProtocol(
        libp2p::Host &host,
        application::ChainSpec const &chain_spec,
        const primitives::BlockHash &genesis_hash,
        std::shared_ptr<parachain::AvailabilityStore> av_store)
        : RequestResponseProtocol<
            FetchAvailableDataRequest,
            FetchAvailableDataResponse,
            ScaleMessageReadWriter>{kName,
                                    host,
                                    make_protocols(kFetchAvailableDataProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(
                                        kName, "req_available_data_protocol")},
          av_store_{std::move(av_store)} {}

   private:
    outcome::result<ResponseType> onRxRequest(
        RequestType candidate_hash, std::shared_ptr<Stream>) override {
      if (auto r = av_store_->getPovAndData(candidate_hash)) {
        return std::move(*r);
      }
      return Empty{};
    }

    void onTxRequest(const RequestType &) override {}

    std::shared_ptr<parachain::AvailabilityStore> av_store_;
  };

  class StatmentFetchingProtocol final
      : public RequestResponseProtocol<FetchStatementRequest,
                                       FetchStatementResponse,
                                       ScaleMessageReadWriter> {
   public:
    static constexpr const char *kName = "FetchStatementProtocol";

    StatmentFetchingProtocol(
        libp2p::Host &host,
        application::ChainSpec const & chain_spec,
        const primitives::BlockHash &genesis_hash,
        std::shared_ptr<parachain::BackingStore> backing_store)
        : RequestResponseProtocol<
            FetchStatementRequest,
            FetchStatementResponse,
            ScaleMessageReadWriter>{kName,
                                    host,
                                    make_protocols(kFetchStatementProtocol,
                                                   genesis_hash,
                                                   "polkadot"),
                                    log::createLogger(kName, "req_statement_protocol")},
          backing_store_{std::move(backing_store)} {}

   private:
    outcome::result<ResponseType> onRxRequest(
        RequestType req, std::shared_ptr<Stream>) override {
      base().logger()->trace(
          "Statement fetch request received.(relay parent={}, candidate "
          "hash={})",
          req.relay_parent,
          req.candidate_hash);
      if (auto res = backing_store_->get_candidate(req.candidate_hash)) {
        return std::move(*res);
      }

      base().logger()->error("No fetch statement response.");
      return outcome::failure(boost::system::error_code{});
    }

    void onTxRequest(const RequestType &) override {}

    std::shared_ptr<parachain::BackingStore> backing_store_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_PROTOCOLS_PROTOCOL_FETCH_AVAILABLE_DATA_HPP
