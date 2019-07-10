/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify_push.hpp"

#include <string>

#include "libp2p/network/listener.hpp"
#include "libp2p/peer/identity_manager.hpp"
#include "libp2p/protocol/identify/utils.hpp"

namespace {
  const std::string kIdentifyPushProtocol = "/ipfs/id/push/1.0.0";
}

namespace libp2p::protocol {
  IdentifyPush::IdentifyPush(std::shared_ptr<Identify> id)
      : id_{std::move(id)} {}

  peer::Protocol IdentifyPush::getProtocolId() const {
    return kIdentifyPushProtocol;
  }

  void IdentifyPush::handle(StreamResult stream_res) {
    if (!stream_res) {
      return;
    }
    id_->receiveIdentify(std::move(stream_res.value()));
  }

  void IdentifyPush::start() {
    id_->bus_.getChannel<network::event::ListenAddressAddedChannel>().subscribe(
        [self{weak_from_this()}](auto && /*ignored*/) {
          if (self.expired()) {
            return;
          }
          self.lock()->sendPush();
        });
    id_->bus_.getChannel<network::event::ListenAddressRemovedChannel>()
        .subscribe([self{weak_from_this()}](auto && /*ignored*/) {
          if (self.expired()) {
            return;
          }
          self.lock()->sendPush();
        });

    id_->bus_.getChannel<peer::event::KeyPairChangedChannel>().subscribe(
        [self{weak_from_this()}](auto && /*ignored*/) {
          if (self.expired()) {
            return;
          }
          self.lock()->sendPush();
        });
  }

  void IdentifyPush::sendPush() {
    detail::streamToEachConnectedPeer(
        id_->host_, id_->conn_manager_, kIdentifyPushProtocol,
        [self{weak_from_this()}](auto &&s) {
          if (auto t = self.lock()) {
            return t->id_->sendIdentify(std::forward<decltype(s)>(s));
          }
        });
  }
}  // namespace libp2p::protocol
