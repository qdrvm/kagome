/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/core_api_provider.hpp"

#include "runtime/wavm/executor.hpp"
#include "runtime/wavm/intrinsic_resolver.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "runtime/wavm/runtime_api/core.hpp"

namespace kagome::runtime::wavm {

  CoreApiProvider::CoreApiProvider(
      std::shared_ptr<runtime::wavm::ModuleRepository> module_repo,
      std::shared_ptr<runtime::wavm::IntrinsicResolver> intrinsic_resolver,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : module_repo_{std::move(module_repo)},
        intrinsic_resolver_{std::move(intrinsic_resolver)},
        storage_provider_{std::move(storage_provider)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)} {
    BOOST_ASSERT(module_repo_);
    BOOST_ASSERT(intrinsic_resolver_);
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(changes_tracker_);
  }

  std::unique_ptr<Core> CoreApiProvider::makeCoreApi(
      gsl::span<uint8_t> runtime_code) const {
    auto new_intrinsic_resolver = intrinsic_resolver_->clone();
    auto executor = std::make_shared<runtime::wavm::Executor>(
        storage_provider_,
        new_intrinsic_resolver->getMemory(),
        module_repo_,
        block_header_repo_);
    return std::make_unique<WavmCore>(
        executor, changes_tracker_, block_header_repo_);
  }

}  // namespace kagome::runtime::wavm
