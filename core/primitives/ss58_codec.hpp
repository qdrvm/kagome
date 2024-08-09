/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "outcome/outcome.hpp"
#include "primitives/account.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::primitives {

  enum class Ss58Error { INVALID_LENGTH = 1, INVALID_CHECKSUM };

  constexpr size_t kSs58ChecksumLength = 2;

  /**
   * Return the account id part of the provided ss58 address. The checksum is
   * verified in the process.
   */
  outcome::result<AccountId> decodeSs58(std::string_view account_address,
                                        const crypto::Hasher &hasher);

  std::string encodeSs58(uint8_t account_type,
                         const AccountId &id,
                         const crypto::Hasher &hasher);

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, Ss58Error);
