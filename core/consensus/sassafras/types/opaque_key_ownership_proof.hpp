/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  /// An opaque type used to represent the key ownership proof at the runtime
  /// API boundary.
  ///
  /// The inner value is an encoded representation of the actual key ownership
  /// proof which will be parameterized when defining the runtime. At the
  /// runtime API boundary this type is unknown and as such we keep this opaque
  /// representation, implementors of the runtime API will have to make sure
  /// that all usages of `OpaqueKeyOwnershipProof` refer to the same type.
  struct OpaqueKeyOwnershipProof {
    SCALE_TIE(1);
    std::vector<uint8_t> data;
  };

}  // namespace kagome::consensus::sassafras
