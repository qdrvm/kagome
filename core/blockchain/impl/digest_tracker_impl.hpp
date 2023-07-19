/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL
#define KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL

#include "blockchain/digest_tracker.hpp"

#include "log/logger.hpp"

namespace kagome::consensus::grandpa {
  class GrandpaDigestObserver;
}

namespace kagome::blockchain {

  class DigestTrackerImpl final : public DigestTracker {
   public:
    DigestTrackerImpl(std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
                          grandpa_digest_observer);

    outcome::result<void> onDigest(const primitives::BlockContext &context,
                                   const primitives::Digest &digest) override;

    void cancel(const primitives::BlockInfo &block) override;

   private:
    outcome::result<void> onConsensus(
        const primitives::BlockContext &context,
        const primitives::Consensus &consensus_message);

    std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
        grandpa_digest_observer_;

    log::Logger logger_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_DIGESTTRACKERIMPL
