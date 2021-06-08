/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/core_api_provider.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/wavm/executor.hpp"
#include "runtime/wavm/impl/crutch.hpp"
#include "runtime/wavm/impl/module_repository_impl.hpp"
#include "runtime/wavm/intrinsic_resolver.hpp"
#include "runtime/wavm/runtime_api/core.hpp"

namespace kagome::runtime::wavm {

  CoreApiProvider::CoreApiProvider(
      std::shared_ptr<runtime::wavm::ModuleRepository> module_repo,
      std::shared_ptr<runtime::wavm::IntrinsicResolver> intrinsic_resolver,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory)
      : module_repo_{std::move(module_repo)},
        intrinsic_resolver_{std::move(intrinsic_resolver)},
        storage_provider_{std::move(storage_provider)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)},
        host_api_factory_{std::move(host_api_factory)} {
    BOOST_ASSERT(module_repo_);
    BOOST_ASSERT(intrinsic_resolver_);
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(changes_tracker_);
    BOOST_ASSERT(host_api_factory_);
  }

  std::unique_ptr<Core> CoreApiProvider::makeCoreApi(
      std::shared_ptr<const crypto::Hasher> hasher,
      gsl::span<uint8_t> runtime_code) const {
    auto new_intrinsic_resolver = std::shared_ptr(intrinsic_resolver_->clone());
    auto new_memory_provider =
        std::make_shared<WavmMemoryProvider>(new_intrinsic_resolver);
    auto executor = std::make_shared<runtime::wavm::Executor>(
        storage_provider_,
        new_memory_provider,
        std::make_shared<ModuleRepositoryImpl>(hasher, new_intrinsic_resolver),
        block_header_repo_);
    executor->setCodeProvider(
        std::make_shared<ConstantCodeProvider>(common::Buffer{runtime_code}));
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        shared_from_this(), new_memory_provider, storage_provider_));
    pushHostApi(host_api);
    executor->setHostApi(host_api);
    return std::make_unique<WavmCore>(
        executor, changes_tracker_, block_header_repo_);
  }

}  // namespace kagome::runtime::wavm
