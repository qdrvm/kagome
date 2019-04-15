/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP_
#define KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP_

#include <variant>
#include "common/blob.hpp"
#include "primitives/common.hpp"

namespace kagome::primitives {
  /// Block id is the variant over BlockHash and BlockNumber
  using BlockId = std::variant<common::Hash256, BlockNumber>;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP_
