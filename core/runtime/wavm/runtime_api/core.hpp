/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_CORE_HPP
#define KAGOME_RUNTIME_WAVM_CORE_HPP

#include "runtime/core.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmCore final : public Core {
   public:
    WavmCore(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
        : executor_{std::move(executor)},
          changes_tracker_{std::move(changes_tracker)},
          header_repo_{std::move(header_repo)} {
      BOOST_ASSERT(executor_ != nullptr);
      BOOST_ASSERT(changes_tracker_ != nullptr);
      BOOST_ASSERT(header_repo_ != nullptr);
    }

    outcome::result<primitives::Version> version(
        const boost::optional<primitives::BlockHash> &block_hash) override {
      return executor_->callAtLatest<primitives::Version>("Core_version",
                                                          block_hash);
    }

    outcome::result<void> execute_block(
        const primitives::Block &block) override {
      OUTCOME_TRY(changes_tracker_->onBlockChange(
          block.header.parent_hash,
          block.header.number - 1));  // parent's number
      return executor_->persistentCallAt<void>(
          {block.header.number - 1, block.header.parent_hash},
          "Core_execute_block",
          block);
    }

    outcome::result<void> initialise_block(
        const primitives::BlockHeader &header) override {
      auto parent = header_repo_->getBlockHeader(header.parent_hash).value();
      OUTCOME_TRY(changes_tracker_->onBlockChange(
          header.parent_hash,
          header.number - 1));  // parent's number
      return executor_->persistentCallAt<void>(
          {header.number - 1, header.parent_hash},
          "Core_initialise_block",
          header);
    }

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override {
      return executor_->callAtLatest<std::vector<primitives::AuthorityId>>(
          "Core_authorities", block_id);
    }

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_RUNTIME_CORE_HPP
