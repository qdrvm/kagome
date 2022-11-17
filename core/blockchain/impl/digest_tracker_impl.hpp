/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL
#define KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL

#include "blockchain/digest_tracker.hpp"

#include "log/logger.hpp"

namespace kagome::authority {
  class AuthorityUpdateObserver;
}
namespace kagome::consensus {
  class BabeDigestObserver;
}

namespace kagome::blockchain {

  class DigestTrackerImpl final : public DigestTracker {
   public:
    DigestTrackerImpl(
        std::shared_ptr<consensus::BabeDigestObserver> babe_update_observer,
        std::shared_ptr<authority::AuthorityUpdateObserver>
            authority_update_observer);

    outcome::result<void> onDigest(const primitives::BlockInfo &block,
                                   const primitives::Digest &digest) override;

    void cancel(const primitives::BlockInfo &block) override;

   private:
    outcome::result<void> onPreRuntime(const primitives::BlockInfo &block,
                                       const primitives::PreRuntime &message);

    outcome::result<void> onConsensus(
        const primitives::BlockInfo &block,
        const primitives::Consensus &consensus_message);

    std::shared_ptr<consensus::BabeDigestObserver> babe_digest_observer_;
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;

    log::Logger logger_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL
