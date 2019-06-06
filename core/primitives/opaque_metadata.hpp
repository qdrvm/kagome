/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
#define KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP

#include <vector>
#include <cstdint>

namespace kagome::primitives {
  using OpaqueMetadata = std::vector<uint8_t>;
}

#endif //KAGOME_CORE_PRIMITIVES_OPAQUE_METADATA_HPP
