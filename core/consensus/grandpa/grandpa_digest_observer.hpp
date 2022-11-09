/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_GRANDPADIGESTOBSERVER
#define KAGOME_CONSENSUS_GRANDPA_GRANDPADIGESTOBSERVER

#include "outcome/outcome.hpp"

#include "primitives/digest.hpp"

namespace kagome::consensus::grandpa {

  class GrandpaDigestObserver {
   public:
    virtual ~GrandpaDigestObserver() = default;

    /**
     * Processes consensus message in block digest
     * @param block - corresponding block
     * @param digest - grandpa digest
     * @return failure or nothing
     */
    virtual outcome::result<void> onDigest(
        const primitives::BlockInfo &block,
        const primitives::GrandpaDigest &message) = 0;

    /**
     * @brief Cancel changes. Should be called when the block is rolled back
     * @param block - corresponding block
     */
    virtual void cancel(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPADIGESTOBSERVER
