/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_AUTHORITY_DISCOVERY_ID_HPP
#define KAGOME_PRIMITIVES_AUTHORITY_DISCOVERY_ID_HPP

#include "crypto/sr25519_types.hpp"

namespace kagome::primitives {
  using AuthorityDiscoveryId = crypto::Sr25519PublicKey;
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_AUTHORITY_DISCOVERY_ID_HPP
