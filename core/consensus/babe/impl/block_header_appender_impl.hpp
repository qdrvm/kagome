/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL
#define KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL

#include "consensus/babe/block_header_appender.hpp"

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

namespace kagome::consensus::babe {

  class BlockAppenderBase;

  class BlockHeaderAppenderImpl
      : public BlockHeaderAppender,
        public std::enable_shared_from_this<BlockHeaderAppenderImpl> {
   public:
    BlockHeaderAppenderImpl(std::shared_ptr<blockchain::BlockTree> block_tree,
                            std::shared_ptr<crypto::Hasher> hasher,
                            std::unique_ptr<BlockAppenderBase> appender);

    outcome::result<void> appendHeader(
        primitives::BlockHeader &&block_header,
        std::optional<primitives::Justification> const &justification) override;

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

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL
