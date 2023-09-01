/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHEIMPL
#define KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHEIMPL

#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {

  class RuntimePropertiesCacheImpl final : public RuntimePropertiesCache {
   public:
    RuntimePropertiesCacheImpl() = default;

    outcome::result<primitives::Version> getVersion(
        const common::Hash256 &hash,
        std::function<outcome::result<primitives::Version>()> obtainer)
        override;

    outcome::result<primitives::OpaqueMetadata> getMetadata(
        const common::Hash256 &hash,
        std::function<outcome::result<primitives::OpaqueMetadata>()> obtainer)
        override;

   private:
    std::map<common::Hash256, primitives::Version> cached_versions_;
    std::map<common::Hash256, primitives::OpaqueMetadata> cached_metadata_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHEIMPL
