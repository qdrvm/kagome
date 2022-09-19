/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_OBSERVER_HPP
#define KAGOME_PARACHAIN_OBSERVER_HPP

#include "network/collation_observer.hpp"
#include "network/req_collation_observer.hpp"

#include <memory>

#include "network/types/collator_messages.hpp"

namespace kagome::network {
  class PeerManager;
}

namespace kagome::observers {
  struct CollationObserverImpl;
  struct ReqCollationObserverImpl;
}  // namespace kagome::observers

namespace kagome::crypto {
  class Sr25519Provider;
}

namespace kagome::parachain {
  struct ParachainProcessorImpl;
}

namespace kagome::parachain {

  struct ParachainObserverImpl final : network::CollationObserver,
                                       network::ReqCollationObserver {
    ParachainObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor);
    ~ParachainObserverImpl() = default;

    /// collation protocol observer
    void onAdvertise(libp2p::peer::PeerId const &peer_id,
                     primitives::BlockHash para_hash) override;
    void onDeclare(libp2p::peer::PeerId const &peer_id,
                   network::CollatorPublicKey pubkey,
                   network::ParachainId para_id,
                   network::Signature signature) override;

    /// fetch collation protocol observer
    outcome::result<network::CollationFetchingResponse> OnCollationRequest(
        network::CollationFetchingRequest request) override;

   private:
    std::shared_ptr<observers::CollationObserverImpl> collation_observer_impl_;
    std::shared_ptr<observers::ReqCollationObserverImpl>
        req_collation_observer_impl_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_OBSERVER_HPP
