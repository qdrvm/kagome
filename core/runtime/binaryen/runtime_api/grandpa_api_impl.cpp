/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/grandpa_api_impl.hpp"

#include "primitives/authority.hpp"

namespace kagome::runtime::binaryen {
  using common::Buffer;
  using primitives::Authority;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;

  GrandpaApiImpl::GrandpaApiImpl(
      const std::shared_ptr<WasmProvider> &wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager,
      const std::shared_ptr<blockchain::BlockHeaderRepository> &header_repo)
      : RuntimeApi(wasm_provider, runtime_manager), header_repo_{header_repo} {
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<boost::optional<ScheduledChange>>
  GrandpaApiImpl::pending_change(const Digest &digest) {
    return execute<boost::optional<ScheduledChange>>(
        "GrandpaApi_grandpa_pending_change",
        CallPersistency::EPHEMERAL,
        digest);
  }

  outcome::result<boost::optional<ForcedChange>> GrandpaApiImpl::forced_change(
      const Digest &digest) {
    return execute<boost::optional<ForcedChange>>(
        "GrandpaApi_grandpa_forced_change", CallPersistency::EPHEMERAL, digest);
  }

  outcome::result<primitives::AuthorityList> GrandpaApiImpl::authorities(
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_id));
    return executeAt<primitives::AuthorityList>(
        "GrandpaApi_grandpa_authorities",
        header.state_root,
        CallPersistency::EPHEMERAL);
  }
}  // namespace kagome::runtime::binaryen
