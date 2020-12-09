/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_METADATA_HPP
#define KAGOME_CORE_RUNTIME_METADATA_HPP

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>
#include "primitives/common.hpp"
#include "primitives/opaque_metadata.hpp"

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
        const boost::optional<primitives::BlockHash> &block_hash) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
