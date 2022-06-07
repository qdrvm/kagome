/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_UPDATE_OBSERVER
#define KAGOME_AUTHORITY_UPDATE_OBSERVER

#include <outcome/outcome.hpp>

#include "primitives/digest.hpp"

namespace kagome::authority {
  class AuthorityUpdateObserver {
   public:
    virtual ~AuthorityUpdateObserver() = default;

    /**
     * Processes consensus message in block digest
     * @param message
     * @return failure or nothing
     */
    virtual outcome::result<void> onConsensus(
        const primitives::BlockInfo &block,
        const primitives::Consensus &message) = 0;

    /**
     * @brief Cancel changes. It is needed rollback in case of failed block
     * @param block rolled back block
     */
    virtual void cancel(const primitives::BlockInfo &block) = 0;
  };
}  // namespace kagome::authority

#endif  // KAGOME_AUTHORITY_UPDATE_OBSERVER
