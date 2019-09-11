/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
#define KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP

#include "ed25519/ed25519.h"

namespace kagome::crypto {

  using ED25519PrivateKey = common::Blob<ed25519_privkey_SIZE>;
  using ED25519PublicKey = common::Blob<ed25519_pubkey_SIZE>;
  using ED25519Signature = common::Blob<ed25519_signature_SIZE>;

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_TYPES_HPP
