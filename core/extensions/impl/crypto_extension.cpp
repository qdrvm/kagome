/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <algorithm>
#include <exception>
#include <gsl/span>

#include <boost/assert.hpp>
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::extensions {
  namespace sr25519_constants = crypto::constants::sr25519;
  namespace ed25519_constants = crypto::constants::ed25519;
  using crypto::decodeKeyTypeId;
  using crypto::secp256k1::CompressedPublicKey;
  using crypto::secp256k1::EcdsaVerifyError;
  using crypto::secp256k1::ExpandedPublicKey;
  using crypto::secp256k1::MessageHash;
  using crypto::secp256k1::RSVSignature;

  CryptoExtension::CryptoExtension(
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<crypto::SR25519Provider> sr25519_provider,
      std::shared_ptr<crypto::ED25519Provider> ed25519_provider,
      std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::CryptoStore> crypto_store,
      std::shared_ptr<crypto::Bip39Provider> bip39_provider)
      : memory_(std::move(memory)),
        sr25519_provider_(std::move(sr25519_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        crypto_store_(std::move(crypto_store)),
        bip39_provider_(std::move(bip39_provider)),
        logger_{common::createLogger("CryptoExtension")} {
    BOOST_ASSERT(memory_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(crypto_store_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
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

  // ---------------------- runtime api version 1 methods ----------------------

  namespace {
    auto encode_result = [](auto &&value) -> common::Buffer {
      static auto logger =
          common::createLogger("CryptoExtension::scale_encode");
      auto &&result = scale::encode(value);
      if (!result) {
        logger->error("failed to scale-encode value: {} ",
                      result.error().message());
        std::terminate();
      }
      return common::Buffer(result.value());
    };

    template <class T>
    auto encode_optional = [](auto &&v) -> common::Buffer {
      return encode_result(std::forward<boost::optional<T>>(v));
    };

    template <class T, class E>
    auto encode_variant = [](auto &&v) -> common::Buffer {
      return encode_result(std::forward<boost::variant<T, E>>(v));
    };
  }  // namespace

  runtime::WasmSpan CryptoExtension::ext_ed25519_public_keys_v1(
      runtime::WasmSize key_type) {
    static const common::Buffer error_result =
        encode_result(std::vector<crypto::ED25519PublicKey>{});

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->warn("key type '{}' is not officially supported ", kt);
    }

    auto &&public_keys = crypto_store_->getEd25519PublicKeys(key_type_id);
    auto buffer = encode_result(public_keys);

    return memory_->storeBuffer(buffer);
  }

  common::Blob<32> CryptoExtension::deriveSeed(
      std::string_view mnemonic_phrase) {
    auto &&mnemonic = crypto::bip39::Mnemonic::parse(mnemonic_phrase);
    if (!mnemonic) {
      logger_->error("failed to parse mnemonic {}", mnemonic.error().message());
      std::terminate();
    }

    auto &&entropy = bip39_provider_->calculateEntropy(mnemonic.value().words);
    if (!entropy) {
      logger_->error("failed to calculate entropy {}",
                     entropy.error().message());
      std::terminate();
    }

    auto &&big_seed =
        bip39_provider_->makeSeed(entropy.value(), mnemonic.value().password);
    if (!big_seed) {
      logger_->error("failed to generate seed {}", big_seed.error().message());
      std::terminate();
    }

    auto big_span = gsl::span<uint8_t>(big_seed.value());
    constexpr size_t size = common::Blob<32>::size();
    // get first 32 bytes from big seed as ed25519 or sr25519 seed
    auto seed = common::Blob<32>::fromSpan(big_span.subspan(0, size));
    if (!seed) {
      // impossible case bip39 seed is always 64 bytes long
      BOOST_UNREACHABLE_RETURN({});
    }
    return seed.value();
  }

  /**
   *@see Extension::ext_ed25519_generate
   */
  runtime::WasmPointer CryptoExtension::ext_ed25519_generate_v1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->warn("key type '{}' is not officially supported", kt);
    }

    auto [seed_ptr, seed_len] = runtime::WasmResult(seed);
    auto &&seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto &&seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      std::terminate();
    }

    crypto::ED25519Keypair kp{};
    boost::optional<std::string> bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto &&ed_seed = deriveSeed(*bip39_seed);
      kp = ed25519_provider_->generateKeypair(ed_seed);
    } else {
      auto &&key_pair = ed25519_provider_->generateKeypair();
      if (!key_pair) {
        logger_->error("failed to generate ed25519 key pair: {}",
                       key_pair.error().message());
        std::terminate();
      }
      kp = key_pair.value();
    }

    common::Buffer buffer(kp.public_key);
    runtime::WasmSpan ps = memory_->storeBuffer(buffer);

    return runtime::WasmResult(ps).address;
  }

  /**
   * @see Extension::ed25519_sign
   */
  runtime::WasmSpan CryptoExtension::ext_ed25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    static const auto kErrorResult =
        encode_optional<crypto::ED25519Signature>(boost::none);

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      logger_->warn("key type '{}' is not supported",
                    decodeKeyTypeId(key_type_id));
    }

    auto public_buffer = memory_->loadN(key, crypto::ED25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    auto msg_buffer = memory_->loadN(msg_data, msg_len);
    auto pk = crypto::ED25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair = crypto_store_->findEd25519Keypair(key_type_id, pk.value());
    if (!key_pair) {
      logger_->error("failed to find required key");
      return memory_->storeBuffer(kErrorResult);
    }

    auto &&sign = ed25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      std::terminate();
    }

    return memory_->storeBuffer(
        encode_optional<crypto::ED25519Signature>(sign.value()));
  }

  /**
   * @see Extension::ext_ed25519_verify
   */
  runtime::WasmSize CryptoExtension::ext_ed25519_verify_v1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    return ext_ed25519_verify(msg_data, msg_len, sig, pubkey_data);
  }

  /**
   * @see Extension::ext_sr25519_public_keys
   */
  runtime::WasmSpan CryptoExtension::ext_sr25519_public_keys_v1(
      runtime::WasmSize key_type) {
    static const common::Buffer error_result =
        encode_result(std::vector<crypto::SR25519PublicKey>{});

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      logger_->warn("key type '{}' is not officially supported",
                    crypto::decodeKeyTypeId(key_type_id));
    }
    auto &&public_keys = crypto_store_->getSr25519PublicKeys(key_type_id);
    auto &&buffer = encode_result(public_keys);

    return memory_->storeBuffer(buffer);
  }

  /**
   *@see Extension::ext_sr25519_generate
   */
  runtime::WasmPointer CryptoExtension::ext_sr25519_generate_v1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->warn("key type '{}' is not officially supported", kt);
    }

    auto [seed_ptr, seed_len] = runtime::WasmResult(seed);
    auto &&seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto &&seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      std::terminate();
    }

    crypto::SR25519Keypair kp{};
    boost::optional<std::string> bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto &&sr_seed = deriveSeed(*bip39_seed);
      kp = sr25519_provider_->generateKeypair(sr_seed);
    } else {
      kp = sr25519_provider_->generateKeypair();
    }

    common::Buffer buffer(kp.public_key);
    runtime::WasmSpan ps = memory_->storeBuffer(buffer);

    return runtime::WasmResult(ps).address;
  }

  /**
   * @see Extension::sr25519_sign
   */
  runtime::WasmSpan CryptoExtension::ext_sr25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    static auto error_result =
        encode_optional<crypto::SR25519Signature>(boost::none);

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->warn("key type '{}' is not officially supported", kt);
    }

    auto public_buffer = memory_->loadN(key, crypto::SR25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    auto msg_buffer = memory_->loadN(msg_data, msg_len);
    auto pk = crypto::SR25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      // error is not possible, since we loaded correct number of bytes
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair = crypto_store_->findSr25519Keypair(key_type_id, pk.value());
    if (!key_pair) {
      logger_->error("failed to find required key: {}",
                     key_pair.error().message());
      return memory_->storeBuffer(error_result);
    }

    auto &&sign = sr25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      std::terminate();
    }

    return memory_->storeBuffer(
        encode_optional<crypto::SR25519Signature>(sign.value()));
  }

  /**
   * @see Extension::ext_sr25519_verify
   */
  runtime::WasmSize CryptoExtension::ext_sr25519_verify_v1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    return ext_sr25519_verify(msg_data, msg_len, sig, pubkey_data);
  }

  // TODO (yuraz): PRE-465 differentiate between 3 types of errors
  runtime::WasmSpan CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    static auto error_result =
        encode_variant<ExpandedPublicKey, EcdsaVerifyError>(
            crypto::secp256k1::ecdsa_verify_error::kInvalidSignature);

    RSVSignature signature{};
    MessageHash message{};
    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    const auto &sig_buffer = memory_->loadN(sig, signature_size);
    const auto &msg_buffer = memory_->loadN(msg, message_size);
    std::copy_n(sig_buffer.begin(), signature_size, signature.begin());
    std::copy_n(msg_buffer.begin(), message_size, message.begin());

    auto &&public_key =
        secp256k1_provider_->recoverPublickeyUncompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key");
      return memory_->storeBuffer(error_result);
    }
    auto &&buffer =
        encode_variant<ExpandedPublicKey, EcdsaVerifyError>(public_key.value());
    return memory_->storeBuffer(buffer);
  }

  // TODO (yuraz): PRE-465 differentiate between 3 types of errors
  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    static auto error_result =
        encode_variant<CompressedPublicKey, EcdsaVerifyError>(
            crypto::secp256k1::ecdsa_verify_error::kInvalidSignature);

    RSVSignature signature{};
    MessageHash message{};
    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    const auto &sig_buffer = memory_->loadN(sig, signature_size);
    const auto &msg_buffer = memory_->loadN(msg, message_size);
    std::copy_n(sig_buffer.begin(), signature_size, signature.begin());
    std::copy_n(msg_buffer.begin(), message_size, message.begin());

    auto &&public_key =
        secp256k1_provider_->recoverPublickeyCompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover compressed secp256k1 public key");
      return memory_->storeBuffer(error_result);
    }

    auto &&buffer = encode_variant<CompressedPublicKey, EcdsaVerifyError>(
        public_key.value());
    return memory_->storeBuffer(buffer);
  }
}  // namespace kagome::extensions
