/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP
#define KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP

#include <boost/variant.hpp>

#include "common/blob.hpp"
#include "primitives/common.hpp"

namespace kagome::primitives {
  using BlockHash = common::Hash256;

  /// Block id is the variant over BlockHash and BlockNumber
  using BlockId = boost::variant<BlockHash, BlockNumber>;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BLOCK_ID_HPP
