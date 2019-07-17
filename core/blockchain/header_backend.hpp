/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP
#define KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP

#include "primitives/block_id.hpp"

namespace kagome::blockchain {

  // TODO(kamilsa): PRE-239 Implement header backend. 11.07.2019
  class HeaderBackend {
   public:
    virtual ~HeaderBackend() = default;
    virtual outcome::result<common::Hash256> hashFromBlockId(
        const primitives::BlockId &block_id) const = 0;

    [[nodiscard]] virtual outcome::result<primitives::BlockNumber>
    numberFromBlockId(const primitives::BlockId &block_id) const = 0;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_HEADER_BACKEND_HPP
