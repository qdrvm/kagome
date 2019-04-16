/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <exception>

#include <ed25519/ed25519.h>
#include <gsl/span>
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
  CryptoExtension::CryptoExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)) {}

  void CryptoExtension::ext_blake2_256(runtime::WasmPointer data,
                                       runtime::SizeType len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    std::array<uint8_t, 32> hash{};
    blake2b(hash.data(), hash.size(), nullptr, 0, buf.toBytes(), buf.size());

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  uint32_t CryptoExtension::ext_ed25519_verify(
      runtime::WasmPointer msg_data, runtime::SizeType msg_len,
      runtime::WasmPointer sig_data, runtime::WasmPointer pubkey_data) {
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kVerifySuccess = 0;
    static constexpr uint32_t kVerifyFail = 5;

    const auto &msg = memory_->loadN(msg_data, msg_len);
    auto *sig_bytes =
        memory_->loadN(sig_data, ed25519_signature_SIZE).toBytes();
    auto signature =
        createEdObject<signature_t, ed25519_signature_SIZE>(sig_bytes);

    auto *pubkey_bytes =
        memory_->loadN(pubkey_data, ed25519_pubkey_SIZE).toBytes();
    auto pubkey =
        createEdObject<public_key_t, ed25519_pubkey_SIZE>(pubkey_bytes);

    return ed25519_verify(&signature, msg.toBytes(), msg_len, &pubkey)
            == ED25519_SUCCESS
        ? kVerifySuccess
        : kVerifyFail;
  }

  void CryptoExtension::ext_twox_128(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    uint8_t *hash = nullptr;
    kagome::crypto::make_twox128(buf.toBytes(), len, hash);
    gsl::span<uint8_t> out(hash, crypto::Twox128Hash::kLength);
    memory_->storeBuffer(out_ptr, common::Buffer(out));
  }

  void CryptoExtension::ext_twox_256(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    uint8_t *hash = nullptr;
    kagome::crypto::make_twox256(buf.toBytes(), len, hash);
    gsl::span<uint8_t> out(hash, crypto::Twox256Hash::kLength);
    memory_->storeBuffer(out_ptr, common::Buffer(out));
  }
}  // namespace kagome::extensions
