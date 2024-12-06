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
                      size_t limit_out,
                      bool collation);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t protocol_group,
                     bool out,
                     Buffer &&handshake) override;
    void onClose(const PeerId &peer_id) override;
    void onClose2(const PeerId &peer_id, bool out) override;

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
    bool collation_;
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

  class ValidationProtocol final : public ParachainProtocol {
   public:
    ValidationProtocol(ParachainProtocolInject inject,
                       std::shared_ptr<ValidationObserver> observer);

    // Controller
    bool onMessage(const PeerId &peer_id,
                   size_t protocol_group,
                   Buffer &&message) override;

    void write(const std::vector<PeerId> &peers,
               const VersionedValidatorProtocolMessage &message);
    void write(const std::unordered_set<PeerId> &peers,
               const VersionedValidatorProtocolMessage &message) {
      write(std::vector(peers.begin(), peers.end()), message);
    }
    void write(const PeerId &peer,
               const VersionedValidatorProtocolMessage &message) {
      write(std::vector{peer}, message);
    }
    void write(const BitfieldDistribution &message);
    void reserve(const PeerId &peer_id, bool add);

   private:
    std::shared_ptr<ValidationObserver> observer_;
  };
}  // namespace kagome::network
