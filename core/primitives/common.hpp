/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_COMMON_HPP
#define KAGOME_CORE_PRIMITIVES_COMMON_HPP

#include <cstdint>

#include "primitives/session_key.hpp"

namespace kagome::primitives {
  using BlockRequestId = uint64_t;

  using BlockNumber = uint64_t;
  using BlockHash = common::Hash256;

  using AuthorityId = SessionKey;
  using AuthorityIndex = uint64_t;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
