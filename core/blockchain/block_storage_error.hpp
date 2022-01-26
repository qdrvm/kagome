/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_BLOCKSTORAGEERROR
#define KAGOME_BLOCKCHAIN_BLOCKSTORAGEERROR

#include "outcome/outcome.hpp"

namespace kagome::blockchain {

  enum class BlockStorageError {
    BLOCK_EXISTS = 1,
    HEADER_DOES_NOT_EXIST,
    BODY_DOES_NOT_EXIST,
    BLOCK_DATA_DOES_NOT_EXIST,
    JUSTIFICATION_DOES_NOT_EXIST,
    GENESIS_BLOCK_ALREADY_EXISTS,
    GENESIS_BLOCK_NOT_FOUND,
    FINALIZED_BLOCK_NOT_FOUND,
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, BlockStorageError);

#endif  // KAGOME_BLOCKCHAIN_BLOCKSTORAGEERROR
