/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_TIMELINEIMPL
#define KAGOME_CONSENSUS_TIMELINEIMPL

#include "consensus/timeline/timeline.hpp"

#include "log/logger.hpp"

namespace kagome::consensus {

  class TimelineImpl final : public Timeline {
   public:
    TimelineImpl();

   private:
    log::Logger log_;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_TIMELINEIMPL
