/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_TWOX_HPP
#define KAGOME_CRYPTO_TWOX_HPP

#include "common/blob.hpp"

namespace kagome::crypto {

  // TODO(warchant): PRE-357 refactor to span

  common::Hash64 make_twox64(gsl::span<const uint8_t> buf);

  common::Hash128 make_twox128(gsl::span<const uint8_t> buf);

  common::Hash256 make_twox256(gsl::span<const uint8_t> buf);

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_TWOX_HPP
