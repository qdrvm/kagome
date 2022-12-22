/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_CORE_HPP
#define KAGOME_RUNTIME_IMPL_CORE_HPP

#include "runtime/runtime_api/core.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {

  class Executor;

  class CoreImpl final : public Core {
   public:
    CoreImpl(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    outcome::result<primitives::Version> version(
        primitives::BlockHash const &block) override;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block) override;

    outcome::result<std::unique_ptr<RuntimeEnvironment>> initialize_block(
        const primitives::BlockHeader &header) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_IMPL_CORE_HPP
