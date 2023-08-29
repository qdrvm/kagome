/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/timeline_impl.hpp"

#include "application/app_state_manager.hpp"

namespace kagome::consensus {

  TimelineImpl::TimelineImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager)
      : log_(log::createLogger("Timeline", "???")),
        app_state_manager_(std::move(app_state_manager)) {
    BOOST_ASSERT(app_state_manager_);

    app_state_manager_->takeControl(*this);
  }
  bool TimelineImpl::prepare() {
    return true;
  }

  bool TimelineImpl::start() {
    return true;
  }

  SyncState TimelineImpl::getCurrentState() const {
    return current_state_;
  }

  bool TimelineImpl::wasSynchronized() const {
    return was_synchronized_;
  }

}  // namespace kagome::consensus
