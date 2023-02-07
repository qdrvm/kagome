/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
#define KAGOME_CONSENSUS_BLOCKAPPENDERMOCK

#include "consensus/babe/block_header_appender.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BlockHeaderAppenderMock : public BlockHeaderAppender {
   public:
    MOCK_METHOD(outcome::result<void>,
                appendHeader,
                (primitives::BlockHeader && block_header,
                 std::optional<primitives::Justification> const &justification),
                (override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDERMOCK
