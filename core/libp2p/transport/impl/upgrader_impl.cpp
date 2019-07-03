/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/upgrader_impl.hpp"

#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::transport, UpgraderImpl::Error, e) {
  using E = libp2p::transport::UpgraderImpl::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::NO_ADAPTOR_FOUND:
      return "can not find suitable adaptor";
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
      std::shared_ptr<protocol_muxer::ProtocolMuxer> protocol_muxer,
      gsl::span<SecAdaptorSPtr> security_adaptors,
      gsl::span<MuxAdaptorSPtr> muxer_adaptors)
      : protocol_muxer_{std::move(protocol_muxer)},
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

  void UpgraderImpl::upgradeToSecureInbound(RawSPtr conn,
                                            OnSecuredCallbackFunc cb) {
    protocol_muxer_->selectOneOf(
        security_protocols_, conn, conn->isInitiator(),
        [self{shared_from_this()}, cb = std::move(cb),
         conn](outcome::result<peer::Protocol> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor =
              findAdaptor(self->security_adaptors_, proto_res.value());
          if (adaptor == nullptr) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          BOOST_ASSERT_MSG(!conn->isInitiator(),
                           "connection is initiator, and SecureInbound is "
                           "called (should be SecureOutbound)");

          return adaptor->secureInbound(std::move(conn), std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToSecureOutbound(RawSPtr conn,
                                             const peer::PeerId &remoteId,
                                             OnSecuredCallbackFunc cb) {
    protocol_muxer_->selectOneOf(
        security_protocols_, conn, conn->isInitiator(),
        [self{shared_from_this()}, cb = std::move(cb), conn,
         &remoteId](outcome::result<peer::Protocol> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor =
              findAdaptor(self->security_adaptors_, proto_res.value());
          if (adaptor == nullptr) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          BOOST_ASSERT_MSG(conn->isInitiator(),
                           "connection is NOT initiator, and SecureOutbound is "
                           "called (should be SecureInbound)");

          return adaptor->secureOutbound(std::move(conn), remoteId,
                                         std::move(cb));
        });
  }

  void UpgraderImpl::upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) {
    return protocol_muxer_->selectOneOf(
        muxer_protocols_, conn, conn->isInitiator(),
        [self{shared_from_this()}, cb = std::move(cb),
         conn](outcome::result<peer::Protocol> proto_res) mutable {
          if (!proto_res) {
            return cb(proto_res.error());
          }

          auto adaptor = findAdaptor(self->muxer_adaptors_, proto_res.value());
          if (!adaptor) {
            return cb(Error::NO_ADAPTOR_FOUND);
          }

          return adaptor->muxConnection(
              std::move(conn),
              [cb = std::move(cb)](outcome::result<CapSPtr> conn_res) {
                if (!conn_res) {
                  return cb(conn_res.error());
                }

                auto &&conn = conn_res.value();
                conn->start();
                return cb(std::move(conn));
              });
        });
  }
}  // namespace libp2p::transport
