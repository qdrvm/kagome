/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include <libp2p/crypto/key.hpp>
#include "common/blob.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;

  enum class KeyTypeError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_KEY_TYPE_ID,
  };

  enum class KeyTypeId : size_t { BABE, GRAN, ACCO, IMON, AUDI };

  using common::Blob;
  using KeyType = Blob<4>;

  outcome::result<KeyType> getKeyTypeById(KeyTypeId key_type_id);

  outcome::result<KeyTypeId> getKeyIdByType(const KeyType &key_type);

  using KeyTypeRepr = uint32_t;
  outcome::result<crypto::KeyTypeId> decodeKeyTypeId(KeyTypeRepr param);
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);

#endif  // KAGOME_CRYPTO_KEY_TYPE_HPP
