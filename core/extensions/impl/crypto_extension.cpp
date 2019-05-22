/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <exception>

extern "C" {
#include <sr25519/sr25519.h>
}
#include <ed25519/ed25519.h>
#include <gsl/span>
#include "crypto/blake2/blake2b.h"
#include "crypto/twox/twox.hpp"

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

  runtime::SizeType CryptoExtension::ext_ed25519_verify(
      runtime::WasmPointer msg_data, runtime::SizeType msg_len,
      runtime::WasmPointer sig_data, runtime::WasmPointer pubkey_data) {
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kVerifySuccess = 0;
    static constexpr uint32_t kVerifyFail = 5;

    const auto &msg = memory_->loadN(msg_data, msg_len);
    auto sig_bytes =
        memory_->loadN(sig_data, ed25519_signature_SIZE).toVector();
    signature_t signature{};
    gsl::span<uint8_t> sig_span(signature.data);
    std::copy(sig_bytes.begin(), sig_bytes.end(), sig_span.begin());

    auto pubkey_bytes =
        memory_->loadN(pubkey_data, ed25519_pubkey_SIZE).toVector();
    public_key_t pubkey{};
    gsl::span<uint8_t> pubkey_span(pubkey.data);
    std::copy(pubkey_bytes.begin(), pubkey_bytes.end(), pubkey_span.begin());

    return ed25519_verify(&signature, msg.toBytes(), msg_len, &pubkey)
            == ED25519_SUCCESS
        ? kVerifySuccess
        : kVerifyFail;
  }

  runtime::SizeType CryptoExtension::ext_sr25519_verify(
      runtime::WasmPointer msg_data, runtime::SizeType msg_len,
      runtime::WasmPointer sig_data, runtime::WasmPointer pubkey_data) {
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kVerifySuccess = 0;
    static constexpr uint32_t kVerifyFail = 5;

    const auto &msg = memory_->loadN(msg_data, msg_len);
    auto signature =
        memory_->loadN(sig_data, SR25519_SIGNATURE_SIZE).toVector();

    auto pubkey =
        memory_->loadN(pubkey_data, SR25519_PUBLIC_SIZE).toVector();

    return sr25519_verify(signature.toBytes(), msg.toBytes(), msg_len, pubkey.toBytes())
        ? kVerifySuccess
        : kVerifyFail;
  }

  void CryptoExtension::ext_twox_128(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto twox128 = kagome::crypto::make_twox128(buf);
    memory_->storeBuffer(out_ptr, common::Buffer(twox128.data));
  }

  void CryptoExtension::ext_twox_256(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto twox256 = kagome::crypto::make_twox256(buf);
    memory_->storeBuffer(out_ptr, common::Buffer(twox256.data));
  }
}  // namespace kagome::extensions
