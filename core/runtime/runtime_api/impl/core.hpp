/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_CORE_HPP
#define KAGOME_RUNTIME_IMPL_CORE_HPP

#include "runtime/runtime_api/core.hpp"

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
        primitives::BlockHash const &block) override;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block,
        TrieChangesTrackerOpt changes_tracker) override;

    outcome::result<std::unique_ptr<RuntimeEnvironment>> initialize_block(
        const primitives::BlockHeader &header,
        TrieChangesTrackerOpt changes_tracker) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_IMPL_CORE_HPP
