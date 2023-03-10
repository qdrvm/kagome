/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_OBSERVER_HPP
#define KAGOME_PARACHAIN_OBSERVER_HPP

#include "network/collation_observer.hpp"
#include "network/req_collation_observer.hpp"
#include "network/req_pov_observer.hpp"
#include "network/validation_observer.hpp"

#include <memory>

#include "network/types/collator_messages.hpp"

namespace kagome::network {
  class PeerManager;
  class PeerView;
}  // namespace kagome::network

namespace kagome::observers {
  struct CollationObserverImpl;
  struct ValidationObserverImpl;
  struct ReqCollationObserverImpl;
  struct ReqPovObserverImpl;
}  // namespace kagome::observers

namespace kagome::crypto {
  class Sr25519Provider;
}

namespace kagome::parachain {
  struct ParachainProcessorImpl;
  struct ApprovalDistribution;
}  // namespace kagome::parachain

namespace kagome::parachain {

  struct ParachainObserverImpl final : network::CollationObserver,
                                       network::ValidationObserver,
                                       network::ReqCollationObserver,
                                       network::ReqPovObserver {
    ParachainObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution);
    ~ParachainObserverImpl() = default;

    /// collation protocol observer
    void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        network::CollationProtocolMessage &&collation_message) override;
    void onIncomingCollationStream(
        libp2p::peer::PeerId const &peer_id) override;

    /// validation protocol observer
    void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        network::ValidatorProtocolMessage &&validation_message) override;
    void onIncomingValidationStream(
        libp2p::peer::PeerId const &peer_id) override;

    /// fetch collation protocol observer
    outcome::result<network::CollationFetchingResponse> OnCollationRequest(
        network::CollationFetchingRequest request) override;

    /// We should response with PoV block if we seconded this candidate
    outcome::result<network::ResponsePov> OnPovRequest(
        network::RequestPov request) override;

   private:
    std::shared_ptr<observers::CollationObserverImpl> collation_observer_impl_;
    std::shared_ptr<observers::ValidationObserverImpl>
        validation_observer_impl_;
    std::shared_ptr<observers::ReqCollationObserverImpl>
        req_collation_observer_impl_;
    std::shared_ptr<observers::ReqPovObserverImpl> req_pov_observer_impl_;
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_OBSERVER_HPP
