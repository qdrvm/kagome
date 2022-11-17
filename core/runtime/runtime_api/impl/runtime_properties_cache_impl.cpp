/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime_properties_cache_impl.hpp"

namespace kagome::runtime {

  outcome::result<primitives::Version> RuntimePropertiesCacheImpl::getVersion(
      const common::Hash256 &hash,
      std::function<outcome::result<primitives::Version>()> obtainer) {
    auto it = cached_versions_.find(hash);
    if (it == cached_versions_.end()) {
      OUTCOME_TRY(version, obtainer());
      it = cached_versions_.emplace(hash, std::move(version)).first;
    }
    return it->second;
  }

  outcome::result<primitives::OpaqueMetadata>
  RuntimePropertiesCacheImpl::getMetadata(
      const common::Hash256 &hash,
      std::function<outcome::result<primitives::OpaqueMetadata>()> obtainer) {
    auto it = cached_metadata_.find(hash);
    if (it == cached_metadata_.end()) {
      OUTCOME_TRY(metadata, obtainer());
      it = cached_metadata_.emplace(hash, std::move(metadata)).first;
    }
    return it->second;
  }

}  // namespace kagome::runtime
