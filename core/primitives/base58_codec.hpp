/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_BASE58_HPP
#define KAGOME_PRIMITIVES_BASE58_HPP

#include <gsl/span>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::primitives {

  enum class Base58Error { INVALID_CHARACTER = 1, NULL_TERMINATOR_NOT_FOUND };

  outcome::result<common::Buffer> decodeBase58(std::string_view str) noexcept;

  std::string encodeBase58(gsl::span<const uint8_t> bytes) noexcept;

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, Base58Error);

#endif  // KAGOME_PRIMITIVES_BASE58_HPP
