/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDERIMPL
#define KAGOME_CONSENSUS_BLOCKAPPENDERIMPL

#include "consensus/timeline/block_header_appender.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus {

  class BlockAppenderBase;

  class BlockHeaderAppenderImpl
      : public BlockHeaderAppender,
        public std::enable_shared_from_this<BlockHeaderAppenderImpl> {
   public:
    BlockHeaderAppenderImpl(std::shared_ptr<blockchain::BlockTree> block_tree,
                            std::shared_ptr<crypto::Hasher> hasher,
                            std::unique_ptr<BlockAppenderBase> appender);

    void appendHeader(
        primitives::BlockHeader &&block_header,
        const std::optional<primitives::Justification> &justification,
        ApplyJustificationCb &&callback) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<crypto::Hasher> hasher_;

    std::optional<primitives::BlockInfo> last_appended_;

    std::unique_ptr<BlockAppenderBase> appender_;

    struct {
      std::chrono::high_resolution_clock::time_point time;
      primitives::BlockNumber block_number;
    } speed_data_ = {};

    log::Logger logger_;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDERIMPL
