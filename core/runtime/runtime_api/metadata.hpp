/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <outcome/outcome.hpp>
#include "primitives/common.hpp"
#include "primitives/metadata.hpp"

namespace kagome::runtime {

  class Metadata {
   protected:
    using OpaqueMetadata = primitives::OpaqueMetadata;

   public:
    virtual ~Metadata() = default;

    /**
     * @brief calls metadata method of Metadata runtime api
     * @return opaque metadata object or error
     */
    virtual outcome::result<OpaqueMetadata> metadata(
        const primitives::BlockHash &block_hash) = 0;
  };

}  // namespace kagome::runtime
