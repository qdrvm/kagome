/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/parachain.hpp"

#include "blockchain/genesis_block_hash.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/notifications/encode.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/validation_observer.hpp"
#include "utils/try.hpp"
#include "utils/weak_macro.hpp"
#include "utils/with_type.hpp"

// TODO(turuslan): https://github.com/qdrvm/kagome/issues/1989
#define PROTOCOL_V1(protocol) \
  {}

namespace kagome::network {
  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/polkadot/node/network/protocol/src/peer_set.rs#L118-L119
  constexpr size_t kCollationPeersLimit = 100;

  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/polkadot/node/network/protocol/src/lib.rs#L47
  constexpr size_t kMinGossipPeers = 25;

  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/polkadot/node/network/protocol/src/peer_set.rs#L98-L99
  constexpr size_t kValidationPeersLimit = kMinGossipPeers / 2 - 1;

  inline auto makeProtocols(const ParachainProtocolInject &inject,
                            const std::string_view &fmt) {
    return make_protocols(fmt, inject.genesis_hash, kProtocolPrefixPolkadot);
  }

  using CollationTypes =
      WithType<vstaging::CollationMessage0, CollationMessage0>;
  using ValidationTypes =
      WithType<vstaging::ValidatorProtocolMessage, ValidatorProtocolMessage>;

  template <typename M>
  auto encodeMessage(const auto &message) {
    return notifications::encode(WireMessage<M>{message});
  }

  inline auto encodeView(const View &view) {
    return encodeMessage<CollationMessage0>(ViewUpdate{view});
  }

  std::pair<size_t, std::shared_ptr<Buffer>> encodeMessage(
      const VersionedValidatorProtocolMessage &message) {
    size_t protocol_group =
        boost::get<vstaging::ValidatorProtocolMessage>(&message) ? 0 : 1;
    auto message_raw = visit_in_place(message, [](const auto &message) {
      return encodeMessage<std::remove_cvref_t<decltype(message)>>(message);
    });
    return {protocol_group, std::move(message_raw)};
  }

  ParachainProtocol::ParachainProtocol(
      ParachainProtocolInject &&inject,
      notifications::ProtocolsGroups protocols_groups,
      size_t limit_in,
      size_t limit_out)
      : notifications_{inject.notifications_factory.make(
          std::move(protocols_groups), limit_in, limit_out)},
        collation_versions_{CollationVersion::VStaging, CollationVersion::V1},
        roles_{inject.roles},
        peer_manager_{inject.peer_manager},
        block_tree_{std::move(inject.block_tree)},
        peer_view_{std::move(inject.peer_view)},
        sync_engine_{std::move(inject.sync_engine)} {}

  Buffer ParachainProtocol::handshake() {
    return scale::encode(roles_).value();
  }

  bool ParachainProtocol::onHandshake(const PeerId &peer_id,
                                      size_t protocol_group,
                                      bool out,
                                      Buffer &&handshake) {
    TRY_FALSE(scale::decode<Roles>(handshake));
    auto state = peer_manager_->createDefaultPeerState(peer_id);
    state.value().get().collation_version =
        collation_versions_.at(protocol_group);
    if (out) {
      notifications_->write(peer_id,
                            protocol_group,
                            encodeView({
                                block_tree_->getLeaves(),
                                block_tree_->getLastFinalized().number,
                            }));
    }
    return true;
  }

  void ParachainProtocol::onClose(const PeerId &peer_id) {}

  void ParachainProtocol::start() {
    if (not roles_.isAuthority()) {
      return;
    }
    if (not sync_sub_) {
      sync_sub_ = primitives::events::onSync(sync_engine_, [WEAK_SELF] {
        WEAK_LOCK(self);
        self->start();
      });
      return;
    }
    notifications_->start(weak_from_this());
    my_view_sub_ =
        primitives::events::subscribe(peer_view_->getMyViewObservable(),
                                      PeerView::EventType::kViewUpdated,
                                      [WEAK_SELF](const ExView &event) {
                                        WEAK_LOCK(self);
                                        self->write(event.view);
                                      });
  }

  void ParachainProtocol::write(const View &view) {
    auto message = encodeView(view);
    notifications_->peersOut([&](const PeerId &peer_id, size_t protocol_group) {
      notifications_->write(peer_id, protocol_group, message);
      return true;
    });
  }

  template <typename Types, typename Observer>
  bool ParachainProtocol::onMessage(const PeerId &peer_id,
                                    size_t protocol_group,
                                    Buffer &&message_raw,
                                    Observer &observer) {
    return Types::with(protocol_group, [&]<typename M>() {
      auto message = TRY_FALSE(scale::decode<WireMessage<M>>(message_raw));
      if (auto *view = boost::get<ViewUpdate>(&message)) {
        peer_view_->updateRemoteView(peer_id, std::move(view->view));
      } else {
        observer.onIncomingMessage(peer_id, std::move(boost::get<M>(message)));
      }
      return true;
    });
  }

  CollationProtocol::CollationProtocol(
      ParachainProtocolInject inject,
      std::shared_ptr<CollationObserver> observer)
      : ParachainProtocol{
          std::move(inject),
          {
              makeProtocols(inject, kCollationProtocolVStaging),
              PROTOCOL_V1(kCollationProtocol),
          },
          kCollationPeersLimit,
          0,
      },
      observer_{std::move(observer)} {}

  bool CollationProtocol::onMessage(const PeerId &peer_id,
                                    size_t protocol_group,
                                    Buffer &&message_raw) {
    return ParachainProtocol::onMessage<CollationTypes>(
        peer_id, protocol_group, std::move(message_raw), *observer_);
  }

  void CollationProtocol::write(const PeerId &peer_id,
                                const Seconded &seconded) {
    auto protocol_group = notifications_->peerOut(peer_id);
    if (not protocol_group) {
      return;
    }
    CollationTypes::with(*protocol_group, [&]<typename M>() {
      notifications_->write(
          peer_id, *protocol_group, encodeMessage<M>(seconded));
    });
  }

  ValidationProtocol::ValidationProtocol(
      ParachainProtocolInject inject,
      std::shared_ptr<ValidationObserver> observer)
      : ParachainProtocol{
          std::move(inject),
          {
              makeProtocols(inject, kValidationProtocolVStaging),
              PROTOCOL_V1(kValidationProtocol),
          },
          kValidationPeersLimit,
          kValidationPeersLimit,
      },
      observer_{std::move(observer)} {}

  bool ValidationProtocol::onMessage(const PeerId &peer_id,
                                     size_t protocol_group,
                                     Buffer &&message_raw) {
    return ParachainProtocol::onMessage<ValidationTypes>(
        peer_id, protocol_group, std::move(message_raw), *observer_);
  }

  void ValidationProtocol::write(
      const PeerId &peer_id,
      std::pair<size_t, std::shared_ptr<Buffer>> message) {
    notifications_->write(peer_id, message.first, std::move(message.second));
  }

  void ValidationProtocol::write(const BitfieldDistribution &message) {
    std::vector<std::shared_ptr<Buffer>> messages;
    for (size_t i = 0; i < collation_versions_.size(); ++i) {
      ValidationTypes::with(i, [&]<typename M>() {
        messages.emplace_back(encodeMessage<M>(message));
      });
    }
    notifications_->peersOut([&](const PeerId &peer_id, size_t protocol_group) {
      notifications_->write(
          peer_id, protocol_group, messages.at(protocol_group));
      return true;
    });
  }

  void ValidationProtocol::reserve(const PeerId &peer_id, bool add) {
    notifications_->reserve(peer_id, add);
  }
}  // namespace kagome::network
