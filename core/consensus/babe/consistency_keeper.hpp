/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPER
#define KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPER

#include "primitives/common.hpp"

namespace kagome::consensus::babe {

  /// Class to provide transactional applying of block and rollback that on
  /// start if last applied block was applied partially
  class ConsistencyKeeper {
   public:
    class Guard final {
     public:
      Guard(ConsistencyKeeper &keeper, primitives::BlockInfo block)
          : keeper_(keeper), block_(block){};
      ~Guard() {
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

    virtual ~ConsistencyKeeper() = default;

    virtual Guard start(primitives::BlockInfo block) = 0;

   protected:
    virtual void commit(primitives::BlockInfo block) = 0;
    virtual void rollback(primitives::BlockInfo block) = 0;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPER
