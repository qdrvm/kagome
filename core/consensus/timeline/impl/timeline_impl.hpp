/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/timeline.hpp"

#include "log/logger.hpp"

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::consensus {

  class TimelineImpl final : public Timeline,
                             public std::enable_shared_from_this<TimelineImpl> {
   public:
    TimelineImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager);

    /// @see AppStateManager::takeControl
    bool prepare();

    /// @see AppStateManager::takeControl
    bool start();

    // Timeline's methods

    SyncState getCurrentState() const override;

    bool wasSynchronized() const override;

   private:
    log::Logger log_;

    std::shared_ptr<application::AppStateManager> app_state_manager_;

    SyncState current_state_{SyncState::WAIT_REMOTE_STATUS};
    bool was_synchronized_{false};
  };

}  // namespace kagome::consensus
