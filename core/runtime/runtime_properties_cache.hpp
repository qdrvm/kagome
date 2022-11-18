/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHE
#define KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHE

#include "common/blob.hpp"
#include "outcome/outcome.hpp"
#include "primitives/opaque_metadata.hpp"
#include "primitives/version.hpp"

namespace kagome::runtime {

  /**
   * Cache for runtime properties (as Version and Metadata)
   * Allows loading and compiling a module directly from its web assembly byte
   * code and instantiating a runtime module at an arbitrary block
   */
  class RuntimePropertiesCache {
   public:
    virtual ~RuntimePropertiesCache() = default;

    virtual outcome::result<primitives::Version> getVersion(
        const common::Hash256 &hash,
        std::function<outcome::result<primitives::Version>()> obtainer) = 0;

    virtual outcome::result<primitives::OpaqueMetadata> getMetadata(
        const common::Hash256 &hash,
        std::function<outcome::result<primitives::OpaqueMetadata>()>
            obtainer) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_RUNTIMEPROPERTIESCACHE
