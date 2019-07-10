/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP
#define KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP

#include "primitives/block_id.hpp"

namespace kagome::blockchain {

  class HeaderBackend {
   public:
    virtual outcome::result<common::Hash256> hashFromBlockId(
        primitives::BlockId block_id) = 0;

    [[nodiscard]] virtual outcome::result<primitives::BlockNumber>
    numberFromBlockId(primitives::BlockId block_id) const = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP
