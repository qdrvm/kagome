/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/core_impl.hpp"

#include <boost/assert.hpp>

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::AuthorityId;
  using primitives::Block;
  using primitives::BlockHeader;
  using primitives::Version;

  CoreImpl::CoreImpl(
      const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory,
      std::shared_ptr<RuntimeCodeProvider> wasm_provider,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : RuntimeApi(runtime_env_factory),
        wasm_provider_{std::move(wasm_provider)},
        changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(wasm_provider_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::versionAt(
      primitives::BlockHash const &block_hash) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    return executeAt<Version>(
        "Core_version",
        header.state_root,
        CallConfig{.persistency = CallPersistency::ISOLATED,
            .runtime_env_config = RuntimeEnvironmentFactory::Config{
                .wasm_provider = wasm_provider_}});
  }

  outcome::result<primitives::Version> CoreImpl::version() {
    return execute<Version>(
        "Core_version",
        CallConfig{.persistency = CallPersistency::ISOLATED,
            .runtime_env_config = RuntimeEnvironmentFactory::Config{
                .wasm_provider = wasm_provider_}});
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block) {
    OUTCOME_TRY(parent, header_repo_->getBlockHeader(block.header.parent_hash));
    OUTCOME_TRY(changes_tracker_->onBlockChange(
        block.header.parent_hash,
        block.header.number - 1));  // parent's number
    return executeAt<void>(
        "Core_execute_block",
        parent.state_root,
        CallConfig{.persistency = CallPersistency::PERSISTENT},
        block);
  }

  outcome::result<void> CoreImpl::initialise_block(const BlockHeader &header) {
    auto parent = header_repo_->getBlockHeader(header.parent_hash).value();
    OUTCOME_TRY(
        changes_tracker_->onBlockChange(header.parent_hash,
                                        header.number - 1));  // parent's number
    return executeAt<void>(
        "Core_initialize_block",
        parent.state_root,
        CallConfig{.persistency = CallPersistency::PERSISTENT},
        header);
  }

  outcome::result<std::vector<AuthorityId>> CoreImpl::authorities(
      const primitives::BlockId &block_id) {
    return execute<std::vector<AuthorityId>>(
        "Core_authorities",
        CallConfig{.persistency = CallPersistency::EPHEMERAL},
        block_id);
  }
}  // namespace kagome::runtime::binaryen
