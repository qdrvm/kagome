/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
#define KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP

#include <cstdint>
#include <vector>

#include <scale/detail/fixed_width_integer.hpp>

#include "scale/tie.hpp"

namespace kagome::primitives {

  /**
   * Polkadot primitive, which is opaque representation of RuntimeMetadata
   */
  using OpaqueMetadata = std::vector<uint8_t>;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
