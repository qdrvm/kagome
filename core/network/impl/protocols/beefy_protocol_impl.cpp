/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/beefy_protocol_impl.hpp"

#include "consensus/beefy/beefy.hpp"
#include "network/common.hpp"
#include "network/notifications/encode.hpp"
#include "utils/try.hpp"

namespace kagome::network {
  using consensus::beefy::BeefyGossipMessage;

  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/substrate/client/consensus/beefy/src/communication/mod.rs#L82-L83
  constexpr size_t kPeersLimit = 25;

  BeefyProtocolImpl::BeefyProtocolImpl(
      const notifications::Factory &notifications_factory,
      const blockchain::GenesisBlockHash &genesis,
      Roles roles,
      std::shared_ptr<Beefy> beefy)
      : notifications_{notifications_factory.make(
          {make_protocols(kBeefyProtocol, genesis)}, kPeersLimit, kPeersLimit)},
        roles_{roles},
        beefy_{std::move(beefy)} {}

  Buffer BeefyProtocolImpl::handshake() {
    return scale::encode(roles_).value();
  }

  bool BeefyProtocolImpl::onHandshake(const PeerId &peer_id,
                                      size_t,
                                      bool,
                                      Buffer &&handshake) {
    TRY_FALSE(scale::decode<Roles>(handshake));
    return true;
  }

  bool BeefyProtocolImpl::onMessage(const PeerId &peer_id,
                                    size_t,
                                    Buffer &&message_raw) {
    auto message = TRY_FALSE(scale::decode<BeefyGossipMessage>(message_raw));
    beefy_->onMessage(std::move(message));
    return true;
  }

  void BeefyProtocolImpl::onClose(const PeerId &peer_id) {}

  void BeefyProtocolImpl::broadcast(
      std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) {
    auto message_raw = notifications::encode(message);
    notifications_->peersOut([&](const PeerId &peer_id, size_t) {
      notifications_->write(peer_id, message_raw);
      return true;
    });
  }

  void BeefyProtocolImpl::start() {
    return notifications_->start(weak_from_this());
  }
}  // namespace kagome::network
