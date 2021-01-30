/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_ACCOUNT_HPP
#define KAGOME_PRIMITIVES_ACCOUNT_HPP

#include "crypto/sr25519_types.hpp"

namespace kagome::primitives {

  using AccountId = crypto::Sr25519PublicKey;

  using AccountNonce = uint32_t;

}

#endif  // KAGOME_PRIMITIVES_ACCOUNT_HPP
