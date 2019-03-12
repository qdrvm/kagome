/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <exception>

#include <ed25519/ed25519.h>
#include "crypto/blake2/blake2b.h"
#include "crypto/twox/twox.hpp"

namespace {
  /**
   * Create an object, needed for Ed25519 methods calls, such as signature or
   * key
   * @tparam EdObject - type of object to be created
   * @tparam InnerArrSize - size of an array inside that object
   * @param data to be copied to that object
   * @return the object
   */
  template <typename EdObject, int InnerArrSize>
  EdObject createEdObject(const uint8_t *data) {
    EdObject out{};
    auto data_end = data;
    std::advance(data_end, InnerArrSize);
    std::copy(data, data_end, static_cast<uint8_t *>(out.data));
    return out;
  }
}  // namespace

namespace kagome::extensions {
  void CryptoExtension::ext_blake2_256(const uint8_t *data, uint32_t len,
                                       uint8_t *out) {
    blake2b(out, 32, nullptr, 0, data, len);
  }

  void CryptoExtension::ext_blake2_256_enumerated_trie_root(
      const uint8_t *values_data, const uint32_t *lens_data,
      uint32_t lens_length, uint8_t *result) {
    // TODO(PRE-54) Akvinikym 11.03.19: implement, when Merkle Trie is ready
    std::terminate();
  }

  uint32_t CryptoExtension::ext_ed25519_verify(const uint8_t *msg_data,
                                               uint32_t msg_len,
                                               const uint8_t *sig_data,
                                               const uint8_t *pubkey_data) {
    auto signature =
        createEdObject<signature_t, ed25519_signature_SIZE>(sig_data);
    auto pubkey =
        createEdObject<public_key_t, ed25519_pubkey_SIZE>(pubkey_data);

    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    return ed25519_verify(&signature, msg_data, msg_len, &pubkey) == 1 ? 0 : 5;
  }

  void CryptoExtension::ext_twox_128(const uint8_t *data, uint32_t len,
                                     uint8_t *out) {
    auto twox = kagome::crypto::make_twox128(data, len);
    std::copy(std::begin(twox.data), std::end(twox.data), out);
  }

  void CryptoExtension::ext_twox_256(const uint8_t *data, uint32_t len,
                                     uint8_t *out) {
    auto twox = kagome::crypto::make_twox256(data, len);
    std::copy(std::begin(twox.data), std::end(twox.data), out);
  }
}  // namespace kagome::extensions
