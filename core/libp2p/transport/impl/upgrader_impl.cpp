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

namespace {
  template <typename AdaptorType>
  std::shared_ptr<AdaptorType> findAdaptor(
      const std::vector<std::shared_ptr<AdaptorType>> &adaptors,
      const libp2p::peer::Protocol &proto) {
    auto adaptor = std::find_if(
        adaptors.begin(), adaptors.end(),
        [&proto](const auto &a) { return proto == a->getProtocolId(); });

    if (adaptor != adaptors.end()) {
      return *adaptor;
    }
    return nullptr;
  }
}  // namespace

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
    // so that we don't need to extract lists of supported protos every time
    security_protocols_ = std::accumulate(
        security_adaptors_.begin(), security_adaptors_.end(),
        std::vector<peer::Protocol>(), [](auto &&acc, const auto &adaptor) {
          acc.push_back(adaptor->getProtocolId());
          return acc;
        });
    muxer_protocols_ = std::accumulate(
        muxer_adaptors.begin(), muxer_adaptors.end(),
        std::vector<peer::Protocol>(), [](auto &&acc, const auto &adaptor) {
          acc.push_back(adaptor->getProtocolId());
          return acc;
        });
  }

  void UpgraderImpl::upgradeToSecure(RawSPtr conn, OnSecuredCallbackFunc cb) {
    auto initiator = conn->isInitiator();

    protocol_muxer_->selectOneOf(
        security_protocols_, conn, initiator,
        [self{shared_from_this()}, cb = std::move(cb), conn,
         initiator](auto &&proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor =
              findAdaptor(self->security_adaptors_, proto_res.value());
          if (!adaptor) {
            return cb(Error::INTERNAL_ERROR);
          }

          return initiator
              ? adaptor->secureOutbound(std::move(conn), self->peer_id_,
                                        std::move(cb))
              : adaptor->secureInbound(std::move(conn), std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) {
    return protocol_muxer_->selectOneOf(
        muxer_protocols_, conn, conn->isInitiator(),
        [self{shared_from_this()}, cb = std::move(cb),
         conn](auto &&proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor = findAdaptor(self->muxer_adaptors_, proto_res.value());
          if (!adaptor) {
            return cb(Error::INTERNAL_ERROR);
          }

          return adaptor->muxConnection(
              std::move(conn), [cb = std::move(cb)](auto &&conn_res) {
                if (!conn_res) {
                  return cb(conn_res.error());
                }

                auto conn = std::move(conn_res.value());
                conn->start();
                return cb(std::move(conn));
              });
        });
  }
}  // namespace libp2p::transport
