/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>

#include "consensus/grandpa/structs.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class JustificationObserver
   * @brief observes justification assigned to syncing blocks.
   */
  class JustificationObserver {
   public:
    virtual ~JustificationObserver() = default;
    using ApplyJustificationCb = std::function<void(outcome::result<void> &&)>;

    /**
     * Validate {@param justification} with {@param authorities}.
     */
    virtual void verifyJustification(
        const GrandpaJustification &justification,
        const primitives::AuthoritySet &authorities,
        std::shared_ptr<std::promise<outcome::result<void>>> promise_res) = 0;

    /**
     * Validate provided {@param justification} for finalization.
     * If it valid finalize block and save {@param justification} in
     * storage.
     * @param justification justification of finalization
     * @return nothing or on error
     */
    virtual void applyJustification(const GrandpaJustification &justification,
                                    ApplyJustificationCb &&callback) = 0;

    /**
     * Reload round after warp sync.
     */
    virtual void reload() = 0;
  };

}  // namespace kagome::consensus::grandpa
