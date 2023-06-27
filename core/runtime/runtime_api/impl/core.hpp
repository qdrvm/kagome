/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_CORE_HPP
#define KAGOME_RUNTIME_IMPL_CORE_HPP

#include "runtime/runtime_api/core.hpp"

#include "common/lru_cache.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {

  class Executor;

  class CoreImpl final : public Core {
   public:
    CoreImpl(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    outcome::result<primitives::Version> version(
        RuntimeEnvironment &env) override;

    outcome::result<primitives::Version> version(
        const primitives::BlockHash &block) override;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block,
        TrieChangesTrackerOpt changes_tracker) override;

    outcome::result<void> execute_block_ref(
        const primitives::BlockReflection &block,
        TrieChangesTrackerOpt changes_tracker) override;

    outcome::result<std::unique_ptr<RuntimeEnvironment>> initialize_block(
        const primitives::BlockHeader &header,
        TrieChangesTrackerOpt changes_tracker) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;

    LruCache<primitives::BlockHash, primitives::Version> version_{10};
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_IMPL_CORE_HPP
