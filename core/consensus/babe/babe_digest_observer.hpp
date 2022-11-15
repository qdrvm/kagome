/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEDIGESTOBSERVER
#define KAGOME_CONSENSUS_BABEDIGESTOBSERVER

#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"

namespace kagome::consensus::babe {

  class BabeDigestObserver {
   public:
    virtual ~BabeDigestObserver() = default;

    /// Observes PreRuntime of block
    /// @param block - block digest of which observed
    /// @param digest - BabeBlockHeader as decoded content of PreRuntime digest
    /// @return failure or nothing
    virtual outcome::result<void> onDigest(const primitives::BlockInfo &block,
                                           const BabeBlockHeader &digest) = 0;

    /// Observes ConsensusLog of block
    /// @param block - block digest of which observed
    /// @param digest - BabeDigest as particular variant of ConsensusLog digest
    /// @return failure or nothing
    virtual outcome::result<void> onDigest(
        const primitives::BlockInfo &block,
        const primitives::BabeDigest &digest) = 0;

    virtual void cancel(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABEDIGESTOBSERVER
