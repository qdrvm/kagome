/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_METADATA_HPP
#define KAGOME_CORE_RUNTIME_METADATA_HPP

#include "primitives/opaque_metadata.hpp"
#include <outcome/outcome.hpp>

namespace kagome::runtime {

class Metadata {
 protected:
  using OpaqueMetadata = primitives::OpaqueMetadata;
 public:
  virtual ~Metadata() = default;

  /**
   * @return opaque metadata object
   */
  virtual outcome::result<OpaqueMetadata> metadata() = 0;

};

}

#endif //KAGOME_CORE_RUNTIME_METADATA_HPP
