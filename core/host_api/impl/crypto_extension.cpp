/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/crypto_extension.hpp"

#include <algorithm>
#include <exception>

#include <boost/assert.hpp>
#include <gsl/span>

#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "crypto/ecdsa_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/memory.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/scale.hpp"

namespace {
  template <typename... Args>
  void throw_with_error(const kagome::log::Logger &logger, Args &&...fmt_args) {
    auto msg = fmt::format(fmt_args...);
    logger->error(msg);
    throw std::runtime_error(msg);
  }

  void checkIfKeyIsSupported(kagome::crypto::KeyTypeId key_type,
                             kagome::log::Logger log) {
    if (not kagome::crypto::isSupportedKeyType(key_type)) {
      const auto *p = reinterpret_cast<const char *>(&key_type);
      std::string key_type_str(p, p + sizeof(key_type));

      log->warn(
          "key type <ascii: '{}', hex: {:08x}> is not officially supported",
          key_type_str,
          key_type);
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
      std::shared_ptr<crypto::CryptoStore> crypto_store)
      : memory_provider_(std::move(memory_provider)),
        sr25519_provider_(std::move(sr25519_provider)),
        ecdsa_provider_(std::move(ecdsa_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        crypto_store_(std::move(crypto_store)),
        logger_{log::createLogger("CryptoExtension", "crypto_extension")} {
    BOOST_ASSERT(memory_provider_ != nullptr);
    BOOST_ASSERT(sr25519_provider_ != nullptr);
    BOOST_ASSERT(ecdsa_provider_ != nullptr);
    BOOST_ASSERT(ed25519_provider_ != nullptr);
    BOOST_ASSERT(secp256k1_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(crypto_store_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
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
    // TODO (kamilsa) 05.07.21 https://github.com/soramitsu/kagome/issues/804
  }

  runtime::WasmSize
  CryptoExtension::ext_crypto_finish_batch_verify_version_1() {
    // TODO (kamilsa) 05.07.21 https://github.com/soramitsu/kagome/issues/804
    return kVerifyBatchSuccess;
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ed25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    using ResultType = std::vector<crypto::Ed25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_keys = crypto_store_->getEd25519PublicKeys(key_type_id);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }
    common::Buffer buffer{scale::encode(public_keys.value()).value()};
    SL_TRACE_FUNC_CALL(logger_, buffer.size(), key_type_id);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmPointer CryptoExtension::ext_crypto_ed25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }
    auto &&seed_opt = seed_res.value();

    outcome::result<crypto::Ed25519Keypair> kp_res{{}};
    if (seed_opt.has_value()) {
      kp_res =
          crypto_store_->generateEd25519Keypair(key_type_id, seed_opt.value());
    } else {
      kp_res = crypto_store_->generateEd25519KeypairOnDisk(key_type_id);
    }
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate ed25519 key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();
    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type_id, seed_buffer);
    runtime::PtrSize res_span{getMemory().storeBuffer(key_pair.public_key)};
    return res_span.combine();
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ed25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::Ed25519Signature>;

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_buffer =
        getMemory().loadN(key, crypto::Ed25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);
    auto pk = crypto::Ed25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair = crypto_store_->findEd25519Keypair(key_type_id, pk.value());
    if (!key_pair) {
      logger_->error("failed to find required key");
      auto error_result = scale::encode(ResultType(std::nullopt)).value();
      return getMemory().storeBuffer(error_result);
    }

    auto sign = ed25519_provider_->sign(key_pair.value(), msg_buffer);
    if (!sign) {
      throw_with_error(
          logger_, "failed to sign message, error = {}", sign.error());
    }
    SL_TRACE_FUNC_CALL(
        logger_, sign.value(), key_pair.value().public_key, msg_buffer);
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
    return ext_crypto_ed25519_verify_version_1(sig, msg_span, pubkey_data);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_sr25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    using ResultType = std::vector<crypto::Sr25519PublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_keys = crypto_store_->getSr25519PublicKeys(key_type_id);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }

    auto buffer = scale::encode(public_keys.value()).value();
    SL_TRACE_FUNC_CALL(logger_, public_keys.value().size(), key_type_id);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmPointer CryptoExtension::ext_crypto_sr25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }

    outcome::result<crypto::Sr25519Keypair> kp_res{{}};
    auto bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      kp_res = crypto_store_->generateSr25519Keypair(key_type_id,
                                                     bip39_seed.value());
    } else {
      kp_res = crypto_store_->generateSr25519KeypairOnDisk(key_type_id);
    }
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate sr25519 key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();

    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type_id, seed_buffer);

    common::Buffer buffer(key_pair.public_key);
    runtime::WasmSpan ps = getMemory().storeBuffer(buffer);

    return runtime::PtrSize(ps).ptr;
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_sr25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::Sr25519Signature>;
    static const auto error_result =
        scale::encode(ResultType(std::nullopt)).value();

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_buffer =
        getMemory().loadN(key, crypto::Sr25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);
    auto pk = crypto::Sr25519PublicKey::fromSpan(public_buffer);
    if (!pk) {
      // error is not possible, since we loaded correct number of bytes
      BOOST_UNREACHABLE_RETURN({});
    }
    auto key_pair = crypto_store_->findSr25519Keypair(key_type_id, pk.value());
    if (!key_pair) {
      logger_->error("failed to find required key: {}", key_pair.error());
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

  int32_t CryptoExtension::ext_crypto_sr25519_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) {
    // TODO(Harrm): this should support deprecated signatures from schnorrkel
    // 0.1.1 in contrary to version_2
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

    auto verify_res = sr25519_provider_->verify_deprecated(signature, msg, key);

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
    return ext_crypto_sr25519_verify_version_1(sig, msg_span, pubkey_data);
  }

  int32_t CryptoExtension::ext_crypto_sr25519_verify_version_2(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) {
    SL_TRACE_FUNC_CALL(logger_,
                       "delegated to ext_crypto_sr25519_verify_version_1");
    return ext_crypto_sr25519_verify_version_1(sig, msg_span, pubkey_data);
  }

  namespace {
    template <typename T>
    using failure_type =
        decltype(outcome::result<std::decay_t<T>>(T{}).as_failure());

    /**
     * @brief converts outcome::failure_type to Secp256k1VerifyError error code
     * @param failure outcome::result containing error
     * @return error code
     */
    template <class T>
    Secp256k1VerifyError convertFailureToError(const failure_type<T> &failure) {
      const outcome::result<void> res = failure;
      if (res == outcome::failure(Secp256k1ProviderError::INVALID_V_VALUE)) {
        return secp256k1::secp256k1_verify_error::kInvalidV;
      }
      if (res
          == outcome::failure(Secp256k1ProviderError::INVALID_R_OR_S_VALUE)) {
        return secp256k1::secp256k1_verify_error::kInvalidRS;
      }

      return secp256k1::secp256k1_verify_error::kInvalidSignature;
    }
  }  // namespace

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    using ResultType =
        boost::variant<secp256k1::PublicKey, Secp256k1VerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = getMemory().loadN(sig, signature_size);
    auto msg_buffer = getMemory().loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key =
        secp256k1_provider_->recoverPublickeyUncompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error());

      auto error_code =
          convertFailureToError<UncompressedPublicKey>(public_key.as_failure());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();

      return getMemory().storeBuffer(error_result);
    }

    // according to substrate implementation
    // returned key shouldn't include the 0x04 prefix
    // specification says, that it should have 64 bytes, not 65 as with prefix
    // On success it contains the 64-byte recovered public key or an error type
    auto truncated_span = gsl::span<uint8_t>(public_key.value()).subspan(1, 64);
    auto truncated_public_key =
        secp256k1::PublicKey::fromSpan(truncated_span).value();
    SL_TRACE_FUNC_CALL(logger_, truncated_public_key, sig_buffer, msg_buffer);
    auto buffer = scale::encode(ResultType(truncated_public_key)).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan
  CryptoExtension::ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    using ResultType =
        boost::variant<CompressedPublicKey, Secp256k1VerifyError>;

    constexpr auto signature_size = RSVSignature::size();
    constexpr auto message_size = MessageHash::size();

    auto sig_buffer = getMemory().loadN(sig, signature_size);
    auto msg_buffer = getMemory().loadN(msg, message_size);

    auto signature = RSVSignature::fromSpan(sig_buffer).value();
    auto message = MessageHash::fromSpan(msg_buffer).value();

    auto public_key =
        secp256k1_provider_->recoverPublickeyCompressed(signature, message);
    if (!public_key) {
      logger_->error("failed to recover uncompressed secp256k1 public key: {}",
                     public_key.error());

      auto error_code =
          convertFailureToError<CompressedPublicKey>(public_key.as_failure());
      auto error_result =
          scale::encode(static_cast<ResultType>(error_code)).value();
      return getMemory().storeBuffer(error_result);
    }
    SL_TRACE_FUNC_CALL(logger_, public_key.value(), sig_buffer, msg_buffer);
    auto buffer = scale::encode(ResultType(public_key.value())).value();
    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ecdsa_public_keys_version_1(
      runtime::WasmSize key_type) {
    using ResultType = std::vector<crypto::EcdsaPublicKey>;
    static const auto error_result(scale::encode(ResultType{}).value());

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_keys = crypto_store_->getEcdsaPublicKeys(key_type_id);
    if (not public_keys) {
      throw_with_error(
          logger_, "error loading public keys: {}", public_keys.error());
    }

    auto buffer = scale::encode(public_keys.value()).value();
    SL_TRACE_FUNC_CALL(logger_, public_keys.value().size(), key_type_id);

    return getMemory().storeBuffer(buffer);
  }

  runtime::WasmSpan CryptoExtension::ext_crypto_ecdsa_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::EcdsaSignature>;

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_buffer = getMemory().loadN(key, sizeof(crypto::EcdsaPublicKey));
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);

    crypto::EcdsaPublicKey pk;
    std::copy(public_buffer.begin(), public_buffer.end(), pk.begin());
    auto key_pair = crypto_store_->findEcdsaKeypair(key_type_id, pk);
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
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    using ResultType = std::optional<crypto::EcdsaSignature>;

    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto public_buffer = getMemory().loadN(key, sizeof(crypto::EcdsaPublicKey));
    auto [msg_data, msg_len] = runtime::PtrSize(msg);
    auto msg_buffer = getMemory().loadN(msg_data, msg_len);

    crypto::EcdsaPublicKey pk;
    std::copy(public_buffer.begin(), public_buffer.end(), pk.begin());
    auto key_pair = crypto_store_->findEcdsaKeypair(key_type_id, pk);
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
      runtime::WasmSize key_type, runtime::WasmSpan seed) const {
    auto key_type_id =
        static_cast<crypto::KeyTypeId>(getMemory().load32u(key_type));
    checkIfKeyIsSupported(key_type_id, logger_);

    auto [seed_ptr, seed_len] = runtime::PtrSize(seed);
    auto seed_buffer = getMemory().loadN(seed_ptr, seed_len);
    auto seed_res = scale::decode<std::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      throw_with_error(logger_, "failed to decode seed");
    }

    outcome::result<crypto::EcdsaKeypair> kp_res{{}};
    auto bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      kp_res =
          crypto_store_->generateEcdsaKeypair(key_type_id, bip39_seed.value());
    } else {
      kp_res = crypto_store_->generateEcdsaKeypairOnDisk(key_type_id);
    }
    if (!kp_res) {
      throw_with_error(
          logger_, "failed to generate ecdsa key pair: {}", kp_res.error());
    }
    auto &key_pair = kp_res.value();

    SL_TRACE_FUNC_CALL(logger_, key_pair.public_key, key_type_id, seed_buffer);

    common::Buffer buffer(key_pair.public_key);
    runtime::WasmSpan ps = getMemory().storeBuffer(buffer);

    return runtime::PtrSize(ps).ptr;
  }

  int32_t CryptoExtension::ext_crypto_ecdsa_verify_version_1(
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

    auto verify_res = ecdsa_provider_->verify(msg, signature, pubkey);

    auto res = verify_res && verify_res.value() ? kVerifySuccess : kVerifyFail;

    SL_TRACE_FUNC_CALL(logger_, res, signature, msg, pubkey);
    return res;
  }

  int32_t CryptoExtension::ext_crypto_ecdsa_verify_version_2(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg_span,
      runtime::WasmPointer pubkey_data) const {
    SL_TRACE_FUNC_CALL(logger_,
                       "delegated to ext_crypto_ecdsa_verify_version_1");
    return ext_crypto_ecdsa_verify_version_1(sig, msg_span, pubkey_data);
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
}  // namespace kagome::host_api
