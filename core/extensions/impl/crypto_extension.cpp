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
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "crypto/key_type.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/typed_key_storage.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"
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
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::storage::TypedKeyStorage> key_storage,
      std::shared_ptr<crypto::Bip39Provider> bip39_provider)
      : memory_(std::move(memory)),
        sr25519_provider_(std::move(sr25519_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        secp256k1_provider_(std::move(secp256k1_provider)),
        hasher_(std::move(hasher)),
        key_storage_(std::move(key_storage)),
        bip39_provider_(std::move(bip39_provider)),
        logger_{common::createLogger("CryptoExtension")} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
    BOOST_ASSERT_MSG(sr25519_provider_ != nullptr,
                     "sr25519 provider is nullptr");
    BOOST_ASSERT_MSG(ed25519_provider_ != nullptr,
                     "ed25519 provider is nullptr");
    BOOST_ASSERT_MSG(secp256k1_provider_ != nullptr, "secp256k1 provider is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(key_storage_ != nullptr, "key storage is nullptr");
    BOOST_ASSERT_MSG(bip39_provider_ != nullptr, "bip39 provider is nullptr");
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

  runtime::WasmSpan CryptoExtension::ext_ed25519_public_keys_v1(
      runtime::WasmSize key_type) {
    static const common::Buffer error_result{};

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->error("key type '{}' is not supported", kt);
      return memory_->storeBuffer(error_result);
    }

    auto &&public_keys = key_storage_->getEd25519Keys(key_type_id);

    auto &&encoded = scale::encode(public_keys);
    if (!encoded) {
      logger_->error("failed to scale-encode vector of public keys: {}",
                     encoded.error().message());
      return memory_->storeBuffer(error_result);
    }

    common::Buffer buffer(std::move(encoded.value()));

    return memory_->storeBuffer(buffer);
  }

  crypto::bip39::Bip39Seed CryptoExtension::deriveBigSeed(
      std::string_view mnemonic_phrase) {
    auto &&mnemonic = crypto::bip39::Mnemonic::parse(mnemonic_phrase);
    if (!mnemonic) {
      logger_->error("failed to parse mnemonic {}", mnemonic.error().message());
      BOOST_ASSERT_MSG(false, "failed to parse mnemonic");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    auto &&entropy = bip39_provider_->calculateEntropy(mnemonic.value().words);
    if (!entropy) {
      logger_->error("failed to calculate entropy {}",
                     entropy.error().message());
      BOOST_ASSERT_MSG(false, "failed to calculate entropy");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    auto &&big_seed =
        bip39_provider_->makeSeed(entropy.value(), mnemonic.value().password);
    if (!big_seed) {
      logger_->error("failed to generate seed {}", big_seed.error().message());
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }
    return big_seed.value();
  }

  /**
   *@see Extension::ext_ed25519_generate
   */
  runtime::WasmPointer CryptoExtension::ext_ed25519_generate_v1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->error("key type '{}' is not supported", kt);
      BOOST_ASSERT_MSG(false, "key type id is not suppirted");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    auto [seed_ptr, seed_len] = runtime::WasmResult(seed);
    auto &&seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto &&seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      BOOST_ASSERT_MSG(false, "failed to decode seed");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    crypto::ED25519Keypair kp{};

    boost::optional<std::string> bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto &&big_seed = deriveBigSeed(*bip39_seed);

      // get first 32 bytes from big seed as ed25519 seed
      auto &&ed_seed = crypto::ED25519Seed::fromSpan(
          gsl::make_span(big_seed).subspan(0, crypto::ED25519Seed::size()));
      if (!ed_seed) {
        logger_->error("failed to get ed25519 seed from span {}",
                       ed_seed.error().message());
        BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
      }

      auto &&key_pair = ed25519_provider_->generateKeyPair(ed_seed.value());
      if (!key_pair) {
        logger_->error("failed to generate ed25519 key pair by seed: {}",
                       key_pair.error().message());
        BOOST_ASSERT_MSG(false, "failed to generate ed25519 key pair");
        BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
      }
      kp = key_pair.value();
    } else {
      auto &&key_pair = ed25519_provider_->generateKeypair();
      if (!key_pair) {
        logger_->error("failed to generate ed25519 key pair: {}",
                       key_pair.error().message());
        BOOST_ASSERT_MSG(false, "failed to generate ed25519 key pair");
        BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
      }
      kp = key_pair.value();
    }

    key_storage_->addEd25519KeyPair(key_type_id, kp);
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
    static auto make_result = [&](auto &&value) {
      boost::optional<crypto::ED25519Signature> result = value;
      return common::Buffer(scale::encode(result).value());
    };
    static const auto kErrorResult = make_result(boost::none);

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->error("key type '{}' is not supported", kt);
      return memory_->storeBuffer(kErrorResult);
    }

    auto public_buffer = memory_->loadN(key, crypto::ED25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    auto msg_buffer = memory_->loadN(msg_data, msg_len);
    // error is not possible, since we loaded correct number of bytes
    auto pk = crypto::ED25519PublicKey::fromSpan(public_buffer).value();
    auto key_pair = key_storage_->findEd25519Keypair(key_type_id, pk);
    if (!key_pair) {
      logger_->error("failed to find required key");
      return memory_->storeBuffer(kErrorResult);
    }

    auto &&sign = ed25519_provider_->sign(*key_pair, msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      return memory_->storeBuffer(kErrorResult);
    }

    return memory_->storeBuffer(make_result(sign.value()));
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
    static const common::Buffer error_result{};
    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      logger_->error("key type '{}' is not supported",
                     crypto::decodeKeyTypeId(key_type_id));
      return memory_->storeBuffer(error_result);
    }
    auto &&public_keys = key_storage_->getSr25519Keys(key_type_id);
    auto &&encoded = scale::encode(public_keys);
    if (!encoded) {
      logger_->error("failed to scale-encode vector of public keys: {}",
                     encoded.error().message());
      return memory_->storeBuffer(error_result);
    }

    common::Buffer buffer(std::move(encoded.value()));

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
      logger_->error("key type '{}' is not supported", kt);
      BOOST_ASSERT_MSG(false, "key type is not supported");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    auto [seed_ptr, seed_len] = runtime::WasmResult(seed);
    auto &&seed_buffer = memory_->loadN(seed_ptr, seed_len);
    auto &&seed_res = scale::decode<boost::optional<std::string>>(seed_buffer);
    if (!seed_res) {
      logger_->error("failed to decode seed");
      BOOST_ASSERT_MSG(false, "failed to decode seed");
      BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
    }

    crypto::SR25519Keypair kp{};

    boost::optional<std::string> bip39_seed = seed_res.value();
    if (bip39_seed.has_value()) {
      auto &&big_seed = deriveBigSeed(*bip39_seed);
      auto &&sr_seed = crypto::ED25519Seed::fromSpan(
          gsl::make_span(big_seed).subspan(0, crypto::SR25519Seed::size()));
      if (!sr_seed) {
        logger_->error("failed to get sr25519 seed from span {}",
                       sr_seed.error().message());
        BOOST_UNREACHABLE_RETURN(runtime::kNullWasmPointer);
      }

      kp = sr25519_provider_->generateKeypair(sr_seed.value());
    } else {
      kp = sr25519_provider_->generateKeypair();
    }

    key_storage_->addSr25519KeyPair(key_type_id, kp);
    common::Buffer pk_buffer(kp.public_key);
    runtime::WasmSpan ps = memory_->storeBuffer(pk_buffer);

    return runtime::WasmResult(ps).address;
  }

  /**
   * @see Extension::sr25519_sign
   */
  runtime::WasmSpan CryptoExtension::ext_sr25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg) {
    static auto make_result = [&](auto &&value) {
      boost::optional<crypto::SR25519Signature> result = value;
      return common::Buffer(scale::encode(result).value());
    };
    static auto error_result = make_result(boost::none);

    auto key_type_id = static_cast<crypto::KeyTypeId>(key_type);
    if (!crypto::isSupportedKeyType(key_type_id)) {
      auto kt = crypto::decodeKeyTypeId(key_type_id);
      logger_->error("key type '{}' is not supported", kt);
      return memory_->storeBuffer(error_result);
    }

    auto public_buffer = memory_->loadN(key, crypto::SR25519PublicKey::size());
    auto [msg_data, msg_len] = runtime::WasmResult(msg);
    auto msg_buffer = memory_->loadN(msg_data, msg_len);
    // error is not possible, since we loaded correct number of bytes
    auto pk = crypto::SR25519PublicKey::fromSpan(public_buffer).value();
    auto key_pair = key_storage_->findSr25519Keypair(key_type_id, pk);
    if (!key_pair) {
      logger_->error("failed to find required key");
      return memory_->storeBuffer(error_result);
    }

    auto &&sign = sr25519_provider_->sign(*key_pair, msg_buffer);
    if (!sign) {
      logger_->error("failed to sign message, error = {}",
                     sign.error().message());
      return memory_->storeBuffer(error_result);
    }

    return memory_->storeBuffer(make_result(sign.value()));
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
