/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_JUSTIFICATIONOBSERVER
#define KAGOME_CONSENSUS_GRANDPA_JUSTIFICATIONOBSERVER

#include "consensus/grandpa/structs.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class JustificationObserver
   * @brief observes justification assigned to syncing blocks.
   */
  struct JustificationObserver {
    virtual ~JustificationObserver() = default;

    /**
     * Validate {@param justification} with {@param authorities}.
     */
    virtual outcome::result<void> verifyJustification(
        const GrandpaJustification &justification,
        const primitives::AuthoritySet &authorities) = 0;

    /**
     * Validate provided {@param justification} for finalization.
     * If it valid finalize block and save {@param justification} in
     * storage.
     * @param justification justification of finalization
     * @return nothing or on error
     */
    virtual outcome::result<void> applyJustification(
        const GrandpaJustification &justification) = 0;

    /**
     * Reload round after warp sync.
     */
    virtual void reload() = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_JUSTIFICATIONOBSERVER
