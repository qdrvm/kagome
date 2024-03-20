/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_store_impl.hpp"

#include <span>

#include "common/bytestr.hpp"
#include "common/visitor.hpp"
#include "utils/read_file.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, KeyStoreError, e) {
  switch (e) {
    using enum kagome::crypto::KeyStoreError;
    case UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
    case UNSUPPORTED_CRYPTO_TYPE:
      return "cryptographic type is not supported";
    case WRONG_SEED_SIZE:
      return "wrong seed size";
    case KEY_NOT_FOUND:
      return "key not found";
    case BABE_ALREADY_EXIST:
      return "BABE key already exists";
    case GRAN_ALREADY_EXIST:
      return "GRAN key already exists";
    case AUDI_ALREADY_EXIST:
      return "AUDI key already exists";
    case WRONG_PUBLIC_KEY:
      return "Public key doesn't match seed";
  }
  return "Unknown KeyStoreError code";
}

namespace kagome::crypto {

}  // namespace kagome::crypto
