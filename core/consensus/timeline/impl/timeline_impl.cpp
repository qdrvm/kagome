/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "timeline_impl.hpp"

namespace kagome::consensus {

  TimelineImpl::TimelineImpl() : log_(log::createLogger("Timeline")){};

}  // namespace kagome::consensus
