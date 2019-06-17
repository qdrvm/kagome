/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/upgrader_impl.hpp"

#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::transport, UpgraderImpl::Error, e) {
  using E = libp2p::transport::UpgraderImpl::Error;
  switch (e) {
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}

namespace libp2p::transport {
  UpgraderImpl::UpgraderImpl(
      peer::PeerId peer_id,
      std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer,
      gsl::span<SecAdaptorSPtr> security_adaptors,
      gsl::span<MuxAdaptorSPtr> muxer_adaptors)
      : peer_id_{std::move(peer_id)},
        protocol_muxer_{std::move(protocol_muxer)},
        security_adaptors_{security_adaptors.begin(), security_adaptors.end()},
        muxer_adaptors_{muxer_adaptors.begin(), muxer_adaptors.end()} {
    // otherwise, we need to extract lists of supported protos every time
    security_protocols_ = std::reduce(
        security_adaptors_.begin(), security_adaptors_.end(),
        std::vector<peer::Protocol>(),
        [](const auto &adaptor) { return adaptor->getProtocolId(); });
    muxer_protocols_ = std::reduce(
        muxer_adaptors.begin(), muxer_adaptors.end(),
        std::vector<peer::Protocol>(),
        [](const auto &adaptor) { return adaptor->getProtocolId(); });
  }

  outcome::result<UpgraderImpl::SecureSPtr> UpgraderImpl::upgradeToSecure(
      RawSPtr conn) {
    auto initiator = conn->isInitiator();
    OUTCOME_TRY(
        selected_proto,
        protocol_muxer_->selectOneOf(security_protocols_, conn, initiator));
    if (auto adaptor =
            std::find_if(security_adaptors_.begin(), security_adaptors_.end(),
                         [&selected_proto](const auto &adaptor) {
                           return selected_proto == adaptor->getProtocolId();
                         });
        adaptor != security_adaptors_.end()) {
      return initiator ? (*adaptor)->secureOutbound(std::move(conn), peer_id_)
                       : (*adaptor)->secureInbound(std::move(conn));
    }
    return Error::INTERNAL_ERROR;
  }

  outcome::result<UpgraderImpl::CapableSPtr> UpgraderImpl::upgradeToMuxed(
      SecureSPtr conn) {
    OUTCOME_TRY(selected_proto,
                protocol_muxer_->selectOneOf(muxer_protocols_, conn,
                                             conn->isInitiator()));
    if (auto adaptor =
            std::find_if(security_adaptors_.begin(), security_adaptors_.end(),
                         [&selected_proto](const auto &adaptor) {
                           return selected_proto == adaptor->getProtocolId();
                         });
        adaptor != security_adaptors_.end()) {
      return (*adaptor)->mux(std::move(conn));
    }
    return Error::INTERNAL_ERROR;
  }
}  // namespace libp2p::transport
