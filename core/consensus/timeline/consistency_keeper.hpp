/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/common.hpp"

namespace kagome::consensus {

  /// Class to provide transactional applying of block and rollback that on
  /// start if last applied block was applied partially
  class ConsistencyKeeper {
    friend class ConsistencyGuard;

   public:
    virtual ~ConsistencyKeeper() = default;

    virtual class ConsistencyGuard start(
        const primitives::BlockInfo &block) = 0;

   protected:
    virtual void commit(const primitives::BlockInfo &block) = 0;
    virtual void rollback(const primitives::BlockInfo &block) = 0;
  };

  class ConsistencyGuard final {
   public:
    ConsistencyGuard(ConsistencyKeeper &keeper,
                     const primitives::BlockInfo &block)
        : keeper_(keeper), block_(block){};

    ConsistencyGuard(const ConsistencyGuard &) = delete;
    ConsistencyGuard &operator=(const ConsistencyGuard &) = delete;

    ConsistencyGuard(ConsistencyGuard &&other) : keeper_{other.keeper_} {
      block_ = other.block_;
      other.block_.reset();
    }
    ConsistencyGuard &operator=(ConsistencyGuard &&other) = delete;

    ~ConsistencyGuard() {
      rollback();
    }

    void commit() {
      if (block_.has_value()) {
        keeper_.commit(block_.value());
        block_.reset();
      }
    }
    void rollback() {
      if (block_.has_value()) {
        keeper_.rollback(block_.value());
        block_.reset();
      }
    }

   private:
    ConsistencyKeeper &keeper_;
    std::optional<primitives::BlockInfo> block_;
  };

}  // namespace kagome::consensus
