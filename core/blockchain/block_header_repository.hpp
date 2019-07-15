/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP
#define KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"

namespace kagome::blockchain {

  enum class BlockStatus { InChain, Unknown };

  class BlockHeaderRepository {
   public:
    virtual ~BlockHeaderRepository() = default;

    virtual auto getNumberByHash(const common::Hash256 &hash) const
        -> outcome::result<boost::optional<primitives::BlockNumber>> = 0;

    virtual auto getHashByNumber(const primitives::BlockNumber &number) const
        -> outcome::result<boost::optional<common::Hash256>> = 0;

    virtual auto getBlockHeader(const primitives::BlockId &id) const
        -> outcome::result<primitives::BlockHeader> = 0;

    virtual outcome::result<bool> getBlockStatus() const = 0;

    auto getNumberById(const primitives::BlockId &id) const
        -> outcome::result<boost::optional<primitives::BlockNumber>> {
      return visit_in_place(id,
                            [](const primitives::BlockNumber &n) {
                              return boost::make_optional(n);
                            },
                            [this](const common::Hash256 &hash) {
                              return getNumberByHash(hash);
                            });
    }

    auto getHashById(const primitives::BlockId &id) const
        -> outcome::result<boost::optional<common::Hash256>> {
      return visit_in_place(id,
                            [this](const primitives::BlockNumber &n) {
                              return getHashByNumber(n);
                            },
                            [](const common::Hash256 &hash) {
                              return boost::make_optional(hash);
                            });
    }
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP
