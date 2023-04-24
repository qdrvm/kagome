/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_GENESISBLOCKHASH
#define KAGOME_BLOCKCHAIN_GENESISBLOCKHASH

#include "blockchain/block_tree.hpp"

namespace kagome::blockchain {

  class BlockTree;

  class GenesisBlockHash final : public primitives::BlockHash {
   public:
    GenesisBlockHash(std::shared_ptr<BlockTree> block_tree);
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_GENESISBLOCKHASH
