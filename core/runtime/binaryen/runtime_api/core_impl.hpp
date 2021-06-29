/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
#define CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/core.hpp"

#include "blockchain/block_header_repository.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::runtime::binaryen {

  class CoreImpl : public RuntimeApi, public Core {
   public:
    CoreImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory,
        std::shared_ptr<RuntimeCodeProvider> wasm_provider,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo);

    ~CoreImpl() override = default;

    outcome::result<primitives::Version> versionAt(
        primitives::BlockHash const &block) override;
    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block) override;

    outcome::result<void> initialise_block(
        const kagome::primitives::BlockHeader &header) override;

    outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) override;

   private:
    std::shared_ptr<RuntimeCodeProvider> wasm_provider_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };
}  // namespace kagome::runtime::binaryen

#endif  // CORE_RUNTIME_BINARYEN_CORE_IMPL_HPP
