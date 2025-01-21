/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/notifications/protocol.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/roles.hpp"
#include "primitives/event_types.hpp"
#include "network/notifications/encode.hpp"

namespace kagome::blockchain {
  class BlockTree;
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::network {
  class CollationObserver;
  enum class CollationVersion;
  class PeerManager;
  class PeerView;
  struct Seconded;
  class ValidationObserver;
  struct View;
}  // namespace kagome::network

namespace kagome::network {
  using libp2p::PeerId;

  std::pair<size_t, std::shared_ptr<Buffer>> encodeMessage(
      const VersionedValidatorProtocolMessage &message);

  template <typename M>
  inline auto encodeMessage(const auto &message) {
    return notifications::encode(WireMessage<M>{message});
  }

  inline auto encodeView(const View &view) {
    return encodeMessage<CollationMessage0>(ViewUpdate{view});
  }

  struct ParachainProtocolInject {
    std::shared_ptr<notifications::Factory> notifications_factory;
    Roles roles;
    std::shared_ptr<blockchain::GenesisBlockHash> genesis_hash;
    std::shared_ptr<PeerManager> peer_manager;
    std::shared_ptr<PeerView> peer_view;
    primitives::events::SyncStateSubscriptionEnginePtr sync_engine;
  };

  struct ParachainProtocol
      : public std::enable_shared_from_this<ParachainProtocol>,
        public notifications::Controller {
    ParachainProtocol(ParachainProtocolInject &&inject,
                      notifications::ProtocolsGroups protocols_groups,
                      size_t limit_in,
                      size_t limit_out);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t protocol_group,
                     bool out,
                     Buffer &&handshake) override;
    void onClose(const PeerId &peer_id) override;

    void start();

   protected:
    template <typename Types, typename Observer>
    bool onMessage(const PeerId &peer_id,
                   size_t protocol_group,
                   Buffer &&message,
                   Observer &observer);
    void write(const View &view);

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    std::shared_ptr<notifications::Protocol> notifications_;
    std::vector<CollationVersion> collation_versions_;
    Roles roles_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<PeerView> peer_view_;
    primitives::events::SyncStateSubscriptionEnginePtr sync_engine_;
    std::shared_ptr<void> sync_sub_;
    std::shared_ptr<void> my_view_sub_;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
  };

  class CollationProtocol final : public ParachainProtocol {
   public:
    CollationProtocol(ParachainProtocolInject inject,
                      std::shared_ptr<CollationObserver> observer);

    // Controller
    bool onMessage(const PeerId &peer_id,
                   size_t protocol_group,
                   Buffer &&message) override;

    void write(const PeerId &peer_id, const Seconded &seconded);

   private:
    std::shared_ptr<CollationObserver> observer_;
  };

  class ValidationProtocolReserve {
   public:
    virtual ~ValidationProtocolReserve() = default;

    virtual void reserve(const PeerId &peer_id, bool add) = 0;
  };

  class ValidationProtocol final : public ParachainProtocol,
                                   public ValidationProtocolReserve {
   public:
    ValidationProtocol(ParachainProtocolInject inject,
                       std::shared_ptr<ValidationObserver> observer);

    // Controller
    bool onMessage(const PeerId &peer_id,
                   size_t protocol_group,
                   Buffer &&message) override;

    void write(const PeerId &peer_id,
               std::pair<size_t, std::shared_ptr<Buffer>> message);
    void write(const PeerId &peer_id,
               const VersionedValidatorProtocolMessage &message) {
      write(peer_id, encodeMessage(message));
    }
    void write(const auto &peers,
               const VersionedValidatorProtocolMessage &message) {
      if (peers.empty()) {
        return;
      }
      auto message_raw = encodeMessage(message);
      for (auto &peer_id : peers) {
        write(peer_id, message_raw);
      }
    }
    void write(const BitfieldDistribution &message);
    void reserve(const PeerId &peer_id, bool add) override;

   private:
    std::shared_ptr<ValidationObserver> observer_;
  };
}  // namespace kagome::network
