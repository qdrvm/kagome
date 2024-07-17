/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/crypto_extension.hpp"

#include <algorithm>
#include <exception>

#include <fmt/format.h>
#include <boost/assert.hpp>
#include <span>

#include "crypto/ecdsa_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_store.hpp"
#include "crypto/key_store/key_type.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519_provider.hpp"
#include "log/trace_macros.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/scale.hpp"

namespace {
  template <typename Format, typename... Args>
  void throw_with_error(const kagome::log::Logger &logger,
                        const Format &format,
                        Args &&...args) {
    auto msg = fmt::vformat(format,
                            fmt::make_format_args(std::forward<Args>(args)...));
    logger->error(msg);
    throw std::runtime_error(msg);
  }

  void checkIfKeyIsSupported(kagome::crypto::KeyType key_type,
                             kagome::log::Logger log) {
    if (not key_type.is_supported()) {
      log->warn("key type {} is not officially supported", key_type);
    }
  }

}  // namespace

namespace kagome::host_api {
  namespace sr25519_constants = crypto::constants::sr25519;
  namespace ed25519_constants = crypto::constants::ed25519;
  namespace ecdsa_constants = crypto::constants::ecdsa;
  namespace secp256k1 = crypto::secp256k1;

  using crypto::Secp256k1ProviderError;
  using secp256k1::CompressedPublicKey;
  using secp256k1::MessageHash;
  using secp256k1::RSVSignature;
  using secp256k1::Secp256k1VerifyError;
  using secp256k1::UncompressedPublicKey;

  CryptoExtension::CryptoExtension(
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<const crypto::EcdsaProvider> ecdsa_provider,
      std::shared_ptr<const crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<const crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::optional<std::shared_ptr<crypto::KeyStore>> key_store)
      : memory_provider_(std::move(memory_provider)),
        sr25519_provider_(std::move(sr25519_provider)),
        ecdsa_provider_(std::move(ecdsa_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        key_store_(std::move(key_store)),
        logger_{log::createLogger("CryptoExtension", "crypto_extension")} {
    BOOST_ASSERT(memory_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ecdsa_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(key_store_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    BOOST_ASSERT(key_store_ == std::nullopt || *key_store_ != nullptr);
  }

  // ---------------------- hashing ----------------------

  runtime::WasmPointer CryptoExtension::ext_hashing_keccak_256_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->keccak_256(buf);

    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_sha2_256_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->sha2_256(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_blake2_128_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->blake2b_128(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_blake2_256_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->blake2b_256(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_64_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->twox_64(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_128_version_1(
      runtime::WasmSpan data) {
    auto [addr, len] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(addr, len);
    auto hash = hasher_->twox_128(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  runtime::WasmPointer CryptoExtension::ext_hashing_twox_256_version_1(
      runtime::WasmSpan data) {
    auto [ptr, size] = runtime::PtrSize(data);
    const auto &buf = getMemory().loadN(ptr, size);
    auto hash = hasher_->twox_256(buf);
    SL_TRACE_FUNC_CALL(logger_, hash, buf);

    return getMemory().storeBuffer(hash);
  }

  void CryptoExtension::ext_crypto_start_batch_verify_version_1() {
    if (batch_verify_) {
      throw_with_error(logger_, "batch already started");
    }
    batch_verify_ = kVerifySuccess;
  }

  runtime::WasmSize
  CryptoExtension::ext_crypto_finish_batch_verify_version_1() {
    if (not batch_verify_) {
      throw_with_error(logger_, "batch not started");
    }
    auto ok = *batch_verify_;
    batch_verify_.reset();
    return ok;
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ed25519_public_keys_version_1(
      runtime::WasmPointer key_type_ptr) {
    using ResultType = std::vector<crypto::Ed25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_keys = key_store_.value()->ed25519().getPublicKeys(key_type);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }
    common::Buffer buffer{scale::encode(public_keys.value()).value()};
    SL_TRACE_FUNC_CALL(logger_, buffer.size(), key_type);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmPointer CryptoExtension::ext_crypto_ed25519_generate_version_1(
      runtime::WasmPointer key_type_ptr, runtime::WasmSpan seed) {
    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }
    auto &&seed_opt = seed_res.value();

    outcome::result<crypto::Ed25519Keypair> kp_res = [&] {
      if (seed_opt.has_value()) {
        return key_store_.value()->ed25519().generateKeypair(key_type,
                                                             seed_opt.value());
      } else {
        return key_store_.value()->ed25519().generateKeypairOnDisk(key_type);
      }
    }();
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate ed25519 key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();
    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type, seed_buffer);
    runtime::PtrSize res_span{getMemory().storeBuffer(key_pair.public_key)};
    return res_span.combine();
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ed25519_sign_version_1(
      runtime::WasmPointer key_type_ptr,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::Ed25519Signature>;

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_buffer =
        getMemory().loadN(key, crypto::Ed25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);
    auto pk = crypto::Ed25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair_opt =
        key_store_.value()->ed25519().findKeypair(key_type, pk.value());
    if (!key_pair_opt) {
      logger_->error("failed to find required key");
      auto error_result = scale::encode(ResultType(std::nullopt)).value();
      return getMemory().storeBuffer(error_result);
    }

    auto sign = ed25519_provider_->sign(key_pair_opt.value(), msg_buffer);
    if (!sign) {
      throw_with_error(
          logger_, "failed to sign message, error = {}", sign.error());
    }
    SL_TRACE_FUNC_CALL(
        logger_, sign.value(), key_pair_opt.value().public_key, msg_buffer);
    auto buffer = scale::encode(ResultType(sign.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSize CryptoExtension::ext_crypto_ed25519_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) {
    auto [msg_data, msg_len] = runtime::PtrSize(msg_span);
    auto msg = getMemory().loadN(msg_data, msg_len);
    auto sig_bytes = getMemory().loadN(sig, ed25519_constants::SIGNATURE_SIZE);

    auto signature_res = crypto::Ed25519Signature::fromSpan(sig_bytes);
    if (!signature_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail);
    }
    auto &&signature = signature_res.value();

    auto pubkey_bytes =
        getMemory().loadN(pubkey_data, ed25519_constants::PUBKEY_SIZE);
    auto pubkey_res = crypto::Ed25519PublicKey::fromSpan(pubkey_bytes);
    if (!pubkey_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail);
    }
    auto pubkey = pubkey_res.value();

    auto verify_res = ed25519_provider_->verify(signature, msg, pubkey);

    auto res = verify_res && verify_res.value() ? kVerifySuccess : kVerifyFail;

    SL_TRACE_FUNC_CALL(logger_, res, signature, msg, pubkey);
    return res;
  }

  runtime::WasmSize CryptoExtension::ext_crypto_ed25519_batch_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) {
    SL_TRACE_FUNC_CALL(
        logger_,
        "Deprecated API method ext_crypto_ed25519_batch_verify_version_1 being "
        "called. Passing call to ext_crypto_ed25519_verify_version_1");
    return batchVerify(
        ext_crypto_ed25519_verify_version_1(sig, msg_span, pubkey_data));
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_sr25519_public_keys_version_1(
      runtime::WasmPointer key_type_ptr) {
    using ResultType = std::vector<crypto::Sr25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_keys = key_store_.value()->sr25519().getPublicKeys(key_type);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }

    auto buffer = scale::encode(public_keys.value()).value();
    SL_TRACE_FUNC_CALL(logger_, public_keys.value().size(), key_type);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmPointer CryptoExtension::ext_crypto_sr25519_generate_version_1(
      runtime::WasmPointer key_type_ptr, runtime::WasmSpan seed) {
    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }

    outcome::result<crypto::Sr25519Keypair> kp_res = [&]() {
      auto bip39_seed = seed_res.value();
      if (bip39_seed.has_value()) {
        return key_store_.value()->sr25519().generateKeypair(
            key_type, bip39_seed.value());
      } else {
        return key_store_.value()->sr25519().generateKeypairOnDisk(key_type);
      }
    }();
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate sr25519 key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();

    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type, seed_buffer);

    common::Buffer buffer(key_pair.public_key);
    runtime::WasmSpan ps = getMemory().storeBuffer(buffer);

    return runtime::PtrSize(ps).ptr;
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_sr25519_sign_version_1(
      runtime::WasmPointer key_type_ptr,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::Sr25519Signature>;
    static const auto error_result =
        scale::encode(ResultType(std::nullopt)).value();

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_buffer =
        getMemory().loadN(key, crypto::Sr25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);
    auto pk = crypto::Sr25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      // error is not possible, since we loaded correct number of bytes
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair =
        key_store_.value()->sr25519().findKeypair(key_type, pk.value());
    if (!key_pair) {
      logger_->error(
          "failed to find required key: {} {}", key_type, pk.value());
      return getMemory().storeBuffer(error_result);
    }

    auto sign = sr25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      throw_with_error(
          logger_, "failed to sign message, error = {}", sign.error());
    }
    SL_TRACE_FUNC_CALL(
        logger_, sign.value(), key_pair.value().public_key, msg_buffer);
    auto buffer = scale::encode(ResultType(sign.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  int32_t CryptoExtension::srVerify(bool deprecated,
                                    runtime::WasmPointer sig,
                                    runtime::WasmSpan msg_span,
                                    runtime::WasmPointer pubkey_data) {
    auto [msg_data, msg_len] = runtime::PtrSize(msg_span);
    auto msg = getMemory().loadN(msg_data, msg_len);
    auto signature_buffer =
        getMemory().loadN(sig, sr25519_constants::SIGNATURE_SIZE);

    auto pubkey_buffer =
        getMemory().loadN(pubkey_data, sr25519_constants::PUBLIC_SIZE);
    auto key_res = crypto::Sr25519PublicKey::fromSpan(pubkey_buffer);
    if (!key_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail)
    }
    auto &&key = key_res.value();

    crypto::Sr25519Signature signature{};
    std::copy_n(signature_buffer.begin(),
                sr25519_constants::SIGNATURE_SIZE,
                signature.begin());

    auto verify_res =
        deprecated ? sr25519_provider_->verify_deprecated(signature, msg, key)
                   : sr25519_provider_->verify(signature, msg, key);

    auto res = verify_res && verify_res.value() ? kVerifySuccess : kVerifyFail;

    SL_TRACE_FUNC_CALL(logger_, res, signature, msg, pubkey_buffer);
    return res;
  }

  int32_t CryptoExtension::ext_crypto_sr25519_batch_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) {
    SL_TRACE_FUNC_CALL(
        logger_,
        "Deprecated API method ext_crypto_sr25519_batch_verify_version_1 being "
        "called. Passing call to ext_crypto_sr25519_verify_version_1");
    return batchVerify(
        ext_crypto_sr25519_verify_version_1(sig, msg_span, pubkey_data));
  }

  int32_t CryptoExtension::ext_crypto_sr25519_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pub) {
    return srVerify(/* deprecated= */ true, sig, msg, pub);
  }

  int32_t CryptoExtension::ext_crypto_sr25519_verify_version_2(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pub) {
    return srVerify(/* deprecated= */ false, sig, msg, pub);
  }

  namespace {
    Secp256k1VerifyError convertFailureToError(const std::error_code &error) {
      if (error == Secp256k1ProviderError::INVALID_V_VALUE) {
        return secp256k1::secp256k1_verify_error::kInvalidV;
      }
      if (error == Secp256k1ProviderError::INVALID_R_OR_S_VALUE) {
        return secp256k1::secp256k1_verify_error::kInvalidRS;
      }

      return secp256k1::secp256k1_verify_error::kInvalidSignature;
    }
  }  // namespace

  runtime::WasmSpan CryptoExtension::ecdsaRecover(bool allow_overflow,
                                                  runtime::WasmPointer sig,
                                                  runtime::WasmPointer msg) {
    using ResultType =
        boost::variant<secp256k1::PublicKey, Secp256k1VerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = getMemory().loadN(sig, signature_size);
    auto msg_buffer = getMemory().loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key = secp256k1_provider_->recoverPublickeyUncompressed(
        signature, message, allow_overflow);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error());

      auto error_code = convertFailureToError(public_key.error());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();

      return getMemory().storeBuffer(error_result);
    }

    // according to substrate implementation
    // returned key shouldn't include the 0x04 prefix
    // specification says, that it should have 64 bytes, not 65 as with prefix
    // On success it contains the 64-byte recovered public key or an error type
    auto truncated_span = std::span<uint8_t>(public_key.value()).subspan(1, 64);
    auto truncated_public_key =
        secp256k1::PublicKey::fromSpan(truncated_span).value();
    SL_TRACE_FUNC_CALL(logger_, truncated_public_key, sig_buffer, msg_buffer);
    auto buffer = scale::encode(ResultType(truncated_public_key)).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return ecdsaRecover(/* allow_overflow= */ true, sig, msg);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_version_2(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return ecdsaRecover(/* allow_overflow= */ false, sig, msg);
  }

  runtime::WasmSpan CryptoExtension::ecdsaRecoverCompressed(
      bool allow_overflow, runtime::WasmPointer sig, runtime::WasmPointer msg) {
    using ResultType =
        boost::variant<CompressedPublicKey, Secp256k1VerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = getMemory().loadN(sig, signature_size);
    auto msg_buffer = getMemory().loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key = secp256k1_provider_->recoverPublickeyCompressed(
        signature, message, allow_overflow);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error());

      auto error_code = convertFailureToError(public_key.error());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();
      return getMemory().storeBuffer(error_result);
    }
    SL_TRACE_FUNC_CALL(logger_, public_key.value(), sig_buffer, msg_buffer);
    auto buffer = scale::encode(ResultType(public_key.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return ecdsaRecoverCompressed(/* allow_overflow= */ true, sig, msg);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_version_2(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return ecdsaRecoverCompressed(/* allow_overflow= */ false, sig, msg);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ecdsa_public_keys_version_1(
      runtime::WasmPointer key_type_ptr) {
    using ResultType = std::vector<crypto::EcdsaPublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_keys = key_store_.value()->ecdsa().getPublicKeys(key_type);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }

    auto buffer = scale::encode(public_keys.value()).value();
    SL_TRACE_FUNC_CALL(logger_, public_keys.value().size(), key_type);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ecdsa_sign_version_1(
      runtime::WasmPointer key_type_ptr,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::EcdsaSignature>;

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_buffer = getMemory().loadN(key, sizeof(crypto::EcdsaPublicKey));
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);

    crypto::EcdsaPublicKey pk;
    std::copy(public_buffer.begin(), public_buffer.end(), pk.begin());
    auto key_pair = key_store_.value()->ecdsa().findKeypair(key_type, pk);
    if (!key_pair) {
      logger_->error("failed to find required key");
      auto error_result = scale::encode(ResultType(std::nullopt)).value();
      return getMemory().storeBuffer(error_result);
    }

    auto sign = ecdsa_provider_->sign(msg_buffer, key_pair.value().secret_key);
    if (!sign) {
      throw_with_error(
          logger_, "failed to sign message, error = {}", sign.error());
    }
    SL_TRACE_FUNC_CALL(
        logger_, sign.value(), key_pair.value().public_key, msg_buffer);
    auto buffer = scale::encode(ResultType(sign.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ecdsa_sign_prehashed_version_1(
      runtime::WasmPointer key_type_ptr,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::EcdsaSignature>;

    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto public_buffer = getMemory().loadN(key, sizeof(crypto::EcdsaPublicKey));
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);

    crypto::EcdsaPublicKey pk;
    std::copy(public_buffer.begin(), public_buffer.end(), pk.begin());
    auto key_pair = key_store_.value()->ecdsa().findKeypair(key_type, pk);
    if (!key_pair) {
      logger_->error("failed to find required key");
      auto error_result = scale::encode(ResultType(std::nullopt)).value();
      return getMemory().storeBuffer(error_result);
    }

    crypto::EcdsaPrehashedMessage digest;
    std::copy(msg_buffer.begin(), msg_buffer.end(), digest.begin());
    auto sign =
        ecdsa_provider_->signPrehashed(digest, key_pair.value().secret_key);
    if (!sign) {
      throw_with_error(
          logger_, "failed to sign message, error = {}", sign.error());
    }
    SL_TRACE_FUNC_CALL(
        logger_, sign.value(), key_pair.value().public_key, msg_buffer);
    auto buffer = scale::encode(ResultType(sign.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmPointer CryptoExtension::ext_crypto_ecdsa_generate_version_1(
      runtime::WasmPointer key_type_ptr, runtime::WasmSpan seed) const {
    crypto::KeyType key_type = loadKeyType(key_type_ptr);
    checkIfKeyIsSupported(key_type, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }

    outcome::result<crypto::EcdsaKeypair> kp_res = [&]() {
      auto bip39_seed = seed_res.value();
      if (bip39_seed.has_value()) {
        return key_store_.value()->ecdsa().generateKeypair(key_type,
                                                           bip39_seed.value());
      } else {
        return key_store_.value()->ecdsa().generateKeypairOnDisk(key_type);
      }
    }();
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate ecdsa key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();

    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type, seed_buffer);

    common::Buffer buffer(key_pair.public_key);
    runtime::WasmSpan ps = getMemory().storeBuffer(buffer);

    return runtime::PtrSize(ps).ptr;
  }

  int32_t CryptoExtension::ecdsaVerify(bool allow_overflow,
                                       runtime::WasmPointer sig,
                                       runtime::WasmSpan msg_span,
                                       runtime::WasmPointer pubkey_data) const {
    auto [msg_data, msg_len] = runtime::PtrSize(msg_span);
    auto msg = getMemory().loadN(msg_data, msg_len);
    auto signature =
        crypto::EcdsaSignature::fromSpan(
            getMemory().loadN(sig, ecdsa_constants::SIGNATURE_SIZE))
            .value();

    auto pubkey_buffer =
        getMemory().loadN(pubkey_data, ecdsa_constants::PUBKEY_SIZE);
    auto key_res = crypto::EcdsaPublicKey::fromSpan(pubkey_buffer);
    if (!key_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail)
    }
    auto &&pubkey = key_res.value();

    auto verify_res =
        ecdsa_provider_->verify(msg, signature, pubkey, allow_overflow);

    auto res = verify_res && verify_res.value() ? kVerifySuccess : kVerifyFail;

    SL_TRACE_FUNC_CALL(logger_, res, signature, msg, pubkey);
    return res;
  }

  int32_t CryptoExtension::ext_crypto_ecdsa_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pub) const {
    return ecdsaVerify(/* allow_overflow= */ true, sig, msg, pub);
  }

  int32_t CryptoExtension::ext_crypto_ecdsa_verify_version_2(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer pub) const {
    return ecdsaVerify(/* allow_overflow= */ false, sig, msg, pub);
  }

  int32_t CryptoExtension::ext_crypto_ecdsa_verify_prehashed_version_1(
      runtime::WasmPointer sig,
      runtime::WasmPointer msg,
      runtime::WasmPointer pubkey_data) const {
    auto signature =
        crypto::EcdsaSignature::fromSpan(
            getMemory().loadN(sig, ecdsa_constants::SIGNATURE_SIZE))
            .value();

    auto pubkey_buffer =
        getMemory().loadN(pubkey_data, ecdsa_constants::PUBKEY_SIZE);
    auto key_res = crypto::EcdsaPublicKey::fromSpan(pubkey_buffer);
    if (!key_res) {
      BOOST_UNREACHABLE_RETURN(kVerifyFail)
    }
    auto &&pubkey = key_res.value();
    auto digest =
        crypto::EcdsaPrehashedMessage::fromSpan(
            getMemory().loadN(msg, crypto::EcdsaPrehashedMessage::size()))
            .value();

    auto verify_res =
        ecdsa_provider_->verifyPrehashed(digest, signature, pubkey);

    auto res = verify_res && verify_res.value() ? kVerifySuccess : kVerifyFail;

    SL_TRACE_FUNC_CALL(logger_, res, signature, msg, pubkey);
    return res;
  }

  void CryptoExtension::reset() {
    batch_verify_.reset();
  }

  runtime::WasmSize CryptoExtension::batchVerify(runtime::WasmSize ok) {
    if (batch_verify_) {
      *batch_verify_ &= ok;
    }
    return ok;
  }

  crypto::KeyType CryptoExtension::loadKeyType(runtime::WasmPointer ptr) const {
    return scale::decode<crypto::KeyType>(
               getMemory().loadN(ptr, sizeof(crypto::KeyType)))
        .value();
  }
}  // namespace kagome::host_api
