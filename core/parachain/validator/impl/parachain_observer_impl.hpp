/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/parachain_observer.hpp"

#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::network {
  class PeerManager;
  class PeerView;
}  // namespace kagome::network

namespace kagome::crypto {
  class Sr25519Provider;
}

namespace kagome::parachain {
  struct ParachainProcessorImpl;
  struct ApprovalDistribution;
}  // namespace kagome::parachain

namespace kagome::parachain {

  class ParachainObserverImpl final : public ParachainObserver {
   public:
    ParachainObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution);

    /// collation protocol observer
    void onIncomingMessage(
        const libp2p::peer::PeerId &peer_id,
        network::CollationProtocolMessage &&collation_message) override;
    void onIncomingCollationStream(
        const libp2p::peer::PeerId &peer_id) override;

    /// validation protocol observer
    void onIncomingMessage(
        const libp2p::peer::PeerId &peer_id,
        network::ValidatorProtocolMessage &&validation_message) override;
    void onIncomingValidationStream(
        const libp2p::peer::PeerId &peer_id) override;

    /// fetch collation protocol observer
    outcome::result<network::CollationFetchingResponse> OnCollationRequest(
        network::CollationFetchingRequest request) override;

    /// We should response with PoV block if we seconded this candidate
    outcome::result<network::ResponsePov> OnPovRequest(
        network::RequestPov request) override;

   private:
    void onAdvertise(const libp2p::peer::PeerId &peer_id,
                     primitives::BlockHash relay_parent);

    void onDeclare(const libp2p::peer::PeerId &peer_id,
                   network::CollatorPublicKey pubkey,
                   network::ParachainId para_id,
                   network::Signature signature);

    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
    std::shared_ptr<network::PeerView> peer_view_;
    std::shared_ptr<parachain::ApprovalDistribution> approval_distribution_;

    log::Logger logger_;
  };

}  // namespace kagome::parachain
