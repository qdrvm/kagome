/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SESSION_KEY_HPP
#define KAGOME_SESSION_KEY_HPP

#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::primitives {

  // TODO(kamilsa): id types should be different for Babe and Grandpa
  using GenericSessionKey = common::Blob<32>;

  using BabeSessionKey = crypto::Sr25519PublicKey;
  using GrandpaSessionKey = crypto::Ed25519PublicKey;

}  // namespace kagome::primitives

#endif  // KAGOME_SESSION_KEY_HPP
