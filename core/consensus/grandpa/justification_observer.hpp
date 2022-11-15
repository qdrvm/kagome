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
     * Validate provided {@param justification} for finalization {@param block}.
     * If it valid finalize {@param block} and save {@param justification} in
     * storage.
     * @param block is observed block info
     * @param justification justification of finalization of provided block
     * @return nothing or on error
     */
    virtual outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const GrandpaJustification &justification) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_JUSTIFICATIONOBSERVER
