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
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"

namespace kagome::extensions {
  namespace sr25519_constants = crypto::constants::sr25519;
  namespace ed25519_constants = crypto::constants::ed25519;
  namespace ecdsa = crypto::secp256k1;

  using crypto::decodeKeyTypeId;
  using crypto::Secp256k1ProviderError;
  using crypto::secp256k1::CompressedPublicKey;
  using crypto::secp256k1::EcdsaVerifyError;
  using crypto::secp256k1::MessageHash;
  using crypto::secp256k1::RSVSignature;
  using crypto::secp256k1::UncompressedPublicKey;

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

    memory_->storeBuffer(out_ptr, hash);
  }

  void CryptoExtension::ext_blake2_256(runtime::WasmPointer data,
                                       runtime::WasmSize len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->blake2b_256(buf);

    memory_->storeBuffer(out_ptr, hash);
  }

  void CryptoExtension::ext_keccak_256(runtime::WasmPointer data,
                                       runtime::WasmSize len,
                                       runtime::WasmPointer out_ptr) {
    const auto &buf = memory_->loadN(data, len);

    auto hash = hasher_->keccak_256(buf);

    memory_->storeBuffer(out_ptr, hash);
  }

  void CryptoExtension::ext_start_batch_verify() {
    if (batch_verify_.has_value()) {
      throw std::runtime_error("Previous batch_verify is not finished");
    }

    batch_verify_.emplace();
  }

  runtime::WasmSize CryptoExtension::ext_finish_batch_verify() {
    if (not batch_verify_.has_value()) {
      throw std::runtime_error("No batch_verify is started");
    }

    auto &verification_queue = batch_verify_.value();
    while (not verification_queue.empty()) {
      auto single_verification_result = verification_queue.front().get();
      if (single_verification_result == 0) {
        batch_verify_.reset();
        return kVerifyBatchFail;
      }
      BOOST_ASSERT_MSG(single_verification_result == 1,
                       "Positive result must be equal 1");
      verification_queue.pop();
    }

    return kVerifyBatchSuccess;
  }

  runtime::WasmSize CryptoExtension::ext_ed25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    auto msg = memory_->loadN(msg_data, msg_len).toVector();
    auto sig_bytes =
        memory_->loadN(sig_data, ed25519_constants::SIGNATURE_SIZE).toVector();
    auto pk_bytes =
        memory_->loadN(pubkey_data, ed25519_constants::PUBKEY_SIZE).toVector();

    auto verifier = [wp = weak_from_this(),
                     msg = std::move(msg),
                     sig_bytes = std::move(sig_bytes),
                     pk_bytes =
                         std::move(pk_bytes)]() mutable -> runtime::WasmSize {
      auto self = wp.lock();
      if (not self) {
        BOOST_ASSERT(!"This is unreachable");
      }

      auto signature_res = crypto::ED25519Signature::fromSpan(sig_bytes);
      if (!signature_res) {
        BOOST_UNREACHABLE_RETURN(kEd25519VerifyFail);
      }
      auto &&signature = signature_res.value();

      auto pubkey_res = crypto::ED25519PublicKey::fromSpan(pk_bytes);
      if (!pubkey_res) {
        BOOST_UNREACHABLE_RETURN(kEd25519VerifyFail);
      }
      auto pubkey = pubkey_res.value();

      auto result = self->ed25519_provider_->verify(signature, msg, pubkey);
      auto is_succeeded = result && result.value();

      return is_succeeded ? kEd25519VerifySuccess : kEd25519VerifyFail;
    };

    if (batch_verify_.has_value()) {
      auto &verification_queue = batch_verify_.value();
      verification_queue.emplace(
          std::async(std::launch::deferred, std::move(verifier)));
      return kEd25519VerifySuccess;
    }

    return verifier();
  }

  runtime::WasmSize CryptoExtension::ext_sr25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    auto msg = memory_->loadN(msg_data, msg_len).toVector();
    auto sig_bytes =
        memory_->loadN(sig_data, sr25519_constants::SIGNATURE_SIZE).toVector();
    auto pk_bytes =
        memory_->loadN(pubkey_data, sr25519_constants::PUBLIC_SIZE).toVector();

    auto verifier = [wp = weak_from_this(),
                     msg = std::move(msg),
                     sig_bytes = std::move(sig_bytes),
                     pk_bytes =
                         std::move(pk_bytes)]() mutable -> runtime::WasmSize {
      auto self = wp.lock();
      if (not self) {
        BOOST_ASSERT(!"This is unreachable");
      }

      auto signature_res = crypto::SR25519Signature::fromSpan(sig_bytes);
      if (!signature_res) {
        BOOST_UNREACHABLE_RETURN(kSr25519VerifyFail);
      }
      auto &&signature = signature_res.value();

      auto pubkey_res = crypto::ED25519PublicKey::fromSpan(pk_bytes);
      if (!pubkey_res) {
        BOOST_UNREACHABLE_RETURN(kSr25519VerifyFail);
      }
      auto pubkey = pubkey_res.value();

      auto result = self->sr25519_provider_->verify(signature, msg, pubkey);
      auto is_succeeded = result && result.value();

      return is_succeeded ? kSr25519VerifySuccess : kSr25519VerifyFail;
    };

    if (batch_verify_.has_value()) {
      auto &verification_queue = batch_verify_.value();
      verification_queue.emplace(
          std::async(std::launch::deferred, std::move(verifier)));
      return kSr25519VerifySuccess;
    }

    return verifier();
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

    memory_->storeBuffer(out_ptr, hash);
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

    memory_->storeBuffer(out_ptr, hash);
  }

  // ---------------------- runtime api version 1 methods ----------------------

  runtime::WasmPointer CryptoExtension::ext_hashing_keccak_256_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->keccak_256(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_sha2_256_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->sha2_256(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_blake2_128_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->blake2b_128(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_blake2_256_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->blake2b_256(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_64_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->twox_64(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_128_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->twox_128(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_256_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::WasmResult(data);
    const auto &buf = memory_->loadN(ptr, size);
    auto hash = hasher_->twox_256(buf);

    return memory_->storeBuffer(hash);
  }

  runtime::WasmSpan CryptoExtension::ext_ed25519_public_keys_v1(
      runtime::WasmSize key_type) {
    using ResultType = std::vector<crypto::ED25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->warn("key type '{}' is not officially supported ", kt);
    }

    auto public_keys = crypto_store_->getEd25519PublicKeys(key_type_id);
    auto buffer = scale::encode(public_keys).value();

    return memory_->storeBuffer(buffer);
  }

  common::Blob<32> CryptoExtension::deriveSeed(std::string_view content) {
    // first check if content is a hexified seed value
    if (auto res = common::Blob<32>::fromHexWithPrefix(content); res) {
      return res.value();
    }

    logger_->debug("failed to unhex seed, try parse mnemonic");

    // now check if it is a bip39 mnemonic phrase with optional password
    auto mnemonic = crypto::bip39::Mnemonic::parse(content);
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
    auto seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      std::terminate();
    }

    crypto::ED25519Keypair kp{};
    boost::optional<std::string> bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto ed_seed = deriveSeed(*bip39_seed);
      kp = ed25519_provider_->generateKeypair(ed_seed);
    } else {
      auto key_pair = ed25519_provider_->generateKeypair();
      if (!key_pair) {
        logger_->error("failed to generate ed25519 key pair: {}",
                       key_pair.error().message());
        std::terminate();
      }
      kp = key_pair.value();
    }

    runtime::WasmSpan ps = memory_->storeBuffer(kp.public_key);

    return runtime::WasmResult(ps).address;
  }

  /**
   * @see Extension::ed25519_sign
   */
  runtime::WasmSpan CryptoExtension::ext_ed25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = boost::optional<crypto::ED25519Signature>;

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
      auto error_result = scale::encode(ResultType(boost::none)).value();
      return memory_->storeBuffer(error_result);
    }

    auto sign = ed25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      std::terminate();
    }

    auto buffer = scale::encode(ResultType(sign.value())).value();
    return memory_->storeBuffer(buffer);
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
    using ResultType = std::vector<crypto::SR25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      logger_->warn("key type '{}' is not officially supported",
                    crypto::decodeKeyTypeId(key_type_id));
    }
    auto public_keys = crypto_store_->getSr25519PublicKeys(key_type_id);
    auto buffer = scale::encode(public_keys).value();

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
    auto seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      std::terminate();
    }

    crypto::SR25519Keypair kp{};
    auto bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto sr_seed = deriveSeed(*bip39_seed);
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
    using ResultType = boost::optional<crypto::SR25519Signature>;
    static const auto error_result =
        scale::encode(ResultType(boost::none)).value();

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

    auto sign = sr25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      std::terminate();
    }
    auto buffer = scale::encode(ResultType(sign.value())).value();
    return memory_->storeBuffer(buffer);
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

  namespace {
    template <typename T>
    using failure_type =
        decltype(outcome::result<std::decay_t<T>>(T{}).as_failure());
    /**
     * @brief converts outcome::failure_type to EcdsaVerifyError error code
     * @param failure outcome::result containing error
     * @return error code
     */
    template <class T>
    EcdsaVerifyError convertFailureToError(const failure_type<T> &failure) {
      const outcome::result<void> res = failure;
      if (res == outcome::failure(Secp256k1ProviderError::INVALID_V_VALUE)) {
        return ecdsa::ecdsa_verify_error::kInvalidV;
      }
      if (res
          == outcome::failure(Secp256k1ProviderError::INVALID_R_OR_S_VALUE)) {
        return ecdsa::ecdsa_verify_error::kInvalidRS;
      }

      return ecdsa::ecdsa_verify_error::kInvalidSignature;
    }
  }  // namespace

  runtime::WasmSpan CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    using ResultType = boost::variant<ecdsa::PublicKey, EcdsaVerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = memory_->loadN(sig, signature_size);
    auto msg_buffer = memory_->loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key =
        secp256k1_provider_->recoverPublickeyUncompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error().message());

      auto error_code =
          convertFailureToError<UncompressedPublicKey>(public_key.as_failure());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();

      return memory_->storeBuffer(error_result);
    }

    // according to substrate implementation
    // returned key shouldn't include the 0x04 prefix
    // specification says, that it should have 64 bytes, not 65 as with prefix
    // On success it contains the 64-byte recovered public key or an error type
    auto truncated_span = gsl::span<uint8_t>(public_key.value()).subspan(1, 64);
    auto truncated_public_key =
        ecdsa::PublicKey::fromSpan(truncated_span).value();
    auto buffer = scale::encode(ResultType(truncated_public_key)).value();
    return memory_->storeBuffer(buffer);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    using ResultType = boost::variant<CompressedPublicKey, EcdsaVerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = memory_->loadN(sig, signature_size);
    auto msg_buffer = memory_->loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key =
        secp256k1_provider_->recoverPublickeyCompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error().message());

      auto error_code =
          convertFailureToError<CompressedPublicKey>(public_key.as_failure());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();
      return memory_->storeBuffer(error_result);
    }

    auto buffer = scale::encode(ResultType(public_key.value())).value();
    return memory_->storeBuffer(buffer);
  }
}  // namespace kagome::extensions
