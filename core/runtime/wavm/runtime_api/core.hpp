/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_CORE_HPP
#define KAGOME_RUNTIME_WAVM_CORE_HPP

#include "runtime/core.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime::wavm {

  class Executor;

  class WavmCore final : public Core {
   public:
    WavmCore(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo);

    outcome::result<primitives::Version> versionAt(primitives::BlockHash const& block) override;
    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_RUNTIME_CORE_HPP
