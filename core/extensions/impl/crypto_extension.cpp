/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <algorithm>
#include <exception>
#include <gsl/span>

#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::extensions {
  namespace sr25519_constants = crypto::constants::sr25519;
  namespace ed25519_constants = crypto::constants::ed25519;

  CryptoExtension::CryptoExtension(
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<crypto::SR25519Provider> sr25519_provider,
      std::shared_ptr<crypto::ED25519Provider> ed25519_provider,
      std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : memory_(std::move(memory)),
        sr25519_provider_(std::move(sr25519_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        logger_{common::createLogger("CryptoExtension")} {
    BOOST_ASSERT(memory_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
  }

  void CryptoExtension::ext_blake2_128(runtime::WasmPointer data,
                                       runtime::WasmSize len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->blake2b_128(buf);

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  void CryptoExtension::ext_blake2_256(runtime::WasmPointer data,
                                       runtime::WasmSize len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->blake2b_256(buf);

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  void CryptoExtension::ext_keccak_256(runtime::WasmPointer data,
                                       runtime::WasmSize len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->keccak_256(buf);

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  runtime::WasmSize CryptoExtension::ext_ed25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kVerifySuccess = 0;
    static constexpr uint32_t kVerifyFail = 5;

    auto msg = memory_->loadN(msg_data, msg_len);
    auto sig_bytes =
        memory_->loadN(sig_data, ed25519_constants::SIGNATURE_SIZE).toVector();

    auto signature_res = crypto::ED25519Signature::fromSpan(sig_bytes);
    if (!signature_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail);
    }
    auto &&signature = signature_res.value();

    auto pubkey_bytes =
        memory_->loadN(pubkey_data, ed25519_constants::PUBKEY_SIZE).toVector();
    auto pubkey_res = crypto::ED25519PublicKey::fromSpan(pubkey_bytes);
    if (!pubkey_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail);
    }
    auto &&pubkey = pubkey_res.value();

    auto result = ed25519_provider_->verify(signature, msg, pubkey);
    auto is_succeeded = result && result.value();

    return is_succeeded ? kVerifySuccess : kVerifyFail;
  }

  runtime::WasmSize CryptoExtension::ext_sr25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kVerifySuccess = 0;
    static constexpr uint32_t kVerifyFail = 5;

    auto msg = memory_->loadN(msg_data, msg_len);
    auto signature_buffer =
        memory_->loadN(sig_data, sr25519_constants::SIGNATURE_SIZE);

    auto pubkey_buffer =
        memory_->loadN(pubkey_data, sr25519_constants::PUBLIC_SIZE);
    auto key_res = crypto::SR25519PublicKey::fromSpan(pubkey_buffer);
    if (!key_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail);
    }
    auto &&key = key_res.value();

    crypto::SR25519Signature signature{};
    std::copy_n(signature_buffer.begin(),
                sr25519_constants::SIGNATURE_SIZE,
                signature.begin());

    auto res = sr25519_provider_->verify(signature, msg, key);
    bool is_succeeded = res && res.value();

    return is_succeeded ? kVerifySuccess : kVerifyFail;
  }

  void CryptoExtension::ext_twox_64(runtime::WasmPointer data,
                                    runtime::WasmSize len,
                                    runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->twox_64(buf);
    logger_->trace("twox64. Data: {}, Data hex: {}, hash: {}",
                   buf.data(),
                   buf.toHex(),
                   hash.toHex());

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  void CryptoExtension::ext_twox_128(runtime::WasmPointer data,
                                     runtime::WasmSize len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->twox_128(buf);
    logger_->trace("twox128. Data: {}, Data hex: {}, hash: {}",
                   buf.data(),
                   buf.toHex(),
                   hash.toHex());

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  void CryptoExtension::ext_twox_256(runtime::WasmPointer data,
                                     runtime::WasmSize len,
                                     runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->twox_256(buf);

    memory_->storeBuffer(out_ptr, common::Buffer(hash));
  }

  namespace {
    template <class T>
    common::Buffer encodeOptionalResult(const boost::optional<T> &value) {
      // TODO (yuraz): PRE-450 refactor
      return common::Buffer(scale::encode(value).value());
    }
  }  // namespace

  runtime::WasmSpan CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    static auto error_result =
        encodeOptionalResult<crypto::secp256k1::ExpandedPublicKey>(boost::none);

    crypto::secp256k1::RSVSignature signature{};
    crypto::secp256k1::MessageHash message{};

    const auto &sig_buffer = memory_->loadN(sig, signature.size());
    const auto &msg_buffer =
        memory_->loadN(msg, crypto::secp256k1::MessageHash::size());
    std::copy_n(sig_buffer.begin(), signature.size(), signature.begin());
    std::copy_n(msg_buffer.begin(),
                crypto::secp256k1::MessageHash::size(),
                message.begin());

    auto &&public_key =
        secp256k1_provider_->recoverPublickeyUncompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key");
      return memory_->storeBuffer(error_result);
    }
    auto &&encoded = scale::encode(public_key.value());
    if (!encoded) {
      logger_->error(
          "failed to scale-encode uncompressed secp256k1 public key: {}",
          encoded.error().message());
      return memory_->storeBuffer(error_result);
    }
    common::Buffer result(std::move(encoded.value()));
    return memory_->storeBuffer(result);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    static auto error_result =
        encodeOptionalResult<crypto::secp256k1::CompressedPublicKey>(
            boost::none);

    crypto::secp256k1::RSVSignature signature{};
    crypto::secp256k1::MessageHash message{};

    const auto &sig_buffer = memory_->loadN(sig, signature.size());
    const auto &msg_buffer =
        memory_->loadN(msg, crypto::secp256k1::MessageHash::size());
    std::copy_n(sig_buffer.begin(), signature.size(), signature.begin());
    std::copy_n(msg_buffer.begin(),
                crypto::secp256k1::MessageHash::size(),
                message.begin());

    auto &&public_key =
        secp256k1_provider_->recoverPublickeyCompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover compressed secp256k1 public key");
      return memory_->storeBuffer(error_result);
    }
    auto &&encoded = scale::encode(public_key.value());
    if (!encoded) {
      logger_->error(
          "failed to scale-encode compressed secp256k1 public key: {}",
          encoded.error().message());
      return memory_->storeBuffer(error_result);
    }
    common::Buffer result(std::move(encoded.value()));
    return memory_->storeBuffer(result);
  }
}  // namespace kagome::extensions
