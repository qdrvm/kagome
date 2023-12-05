/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/block_header_appender.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockHeaderAppenderMock : public BlockHeaderAppender {
   public:
    MOCK_METHOD(void,
                appendHeader,
                (primitives::BlockHeader && block_header,
                 const std::optional<primitives::Justification> &justification,
                 ApplyJustificationCb &&callback),
                (override));
  };

}  // namespace kagome::consensus
