/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_COMMON_HPP
#define KAGOME_CORE_PRIMITIVES_COMMON_HPP

#include <cstdint>

#include "common/blob.hpp"

namespace kagome::primitives {
  using BlockNumber = uint64_t;
  using BlockHash = common::Hash256;

  // TODO(akvinikym): must be a SR25519 key
  using AuthorityId = common::Blob<32>;
  using AuthorityIndex = uint64_t;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
