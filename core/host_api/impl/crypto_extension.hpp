/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_EXTENSION_HPP
#define KAGOME_CRYPTO_EXTENSION_HPP

#include <future>
#include <optional>
#include <queue>

#include "crypto/bip39/bip39_types.hpp"
#include "crypto/crypto_store.hpp"
#include "log/logger.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::crypto {
  class Sr25519Provider;
  class Ed25519Provider;
  class Secp256k1Provider;
  class Hasher;
  class Bip39Provider;
  class CryptoStore;
}  // namespace kagome::crypto

namespace kagome::host_api {
  /**
   * Implements extension functions related to cryptography
   */
  class CryptoExtension : public std::enable_shared_from_this<CryptoExtension> {
   public:
    // for some reason, 0 and 5 are used in the reference implementation, so
    // it's better to stick to them in ours, at least for now
    static constexpr uint32_t kLegacyVerifySuccess = 0;
    static constexpr uint32_t kLegacyVerifyFail = 5;

    static constexpr uint32_t kVerifyBatchSuccess = 1;
    static constexpr uint32_t kVerifyBatchFail = 0;
    static constexpr uint32_t kVerifySuccess = 1;
    static constexpr uint32_t kVerifyFail = 0;

    CryptoExtension(
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::CryptoStore> crypto_store,
        std::shared_ptr<crypto::Bip39Provider> bip39_provider);

    inline void reset() {
      batch_verify_ = boost::none;
    }

    // -------------------- hashing methods v1 --------------------

    /**
     * @see Extension::ext_hashing_keccak_256_version_1
     */
    runtime::WasmPointer ext_hashing_keccak_256_version_1(
        runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_sha2_256_version_1
     */
    runtime::WasmPointer ext_hashing_sha2_256_version_1(runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_blake2_128_version_1
     */
    runtime::WasmPointer ext_hashing_blake2_128_version_1(
        runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_blake2_256_version_1
     */
    runtime::WasmPointer ext_hashing_blake2_256_version_1(
        runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_twox_64_version_1
     */
    runtime::WasmPointer ext_hashing_twox_64_version_1(runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_twox_128_version_1
     */
    runtime::WasmPointer ext_hashing_twox_128_version_1(runtime::WasmSpan data);

    /**
     * @see Extension::ext_hashing_twox_256_version_1
     */
    runtime::WasmPointer ext_hashing_twox_256_version_1(runtime::WasmSpan data);

    // -------------------- crypto methods v1 --------------------

    /**
     * @see Extension::ext_ed25519_public_keys
     */
    runtime::WasmSpan ext_ed25519_public_keys_version_1(runtime::WasmSize key_type);

    /**
     *@see Extension::ext_ed25519_generate
     */
    runtime::WasmPointer ext_ed25519_generate_version_1(runtime::WasmSize key_type,
                                                 runtime::WasmSpan seed);

    /**
     * @see Extension::ed25519_sign
     */
    runtime::WasmSpan ext_ed25519_sign_version_1(runtime::WasmSize key_type,
                                          runtime::WasmPointer key,
                                          runtime::WasmSpan msg);

    /**
     * @see Extension::ext_ed25519_verify
     */
    runtime::WasmSize ext_ed25519_verify_version_1(runtime::WasmPointer sig,
                                            runtime::WasmSpan msg,
                                            runtime::WasmPointer pubkey_data);

    /**
     * @see Extension::ext_sr25519_public_keys
     */
    runtime::WasmSpan ext_sr25519_public_keys_version_1(runtime::WasmSize key_type);

    /**
     *@see Extension::ext_sr25519_generate
     */
    runtime::WasmPointer ext_sr25519_generate_version_1(runtime::WasmSize key_type,
                                                 runtime::WasmSpan seed);

    /**
     * @see Extension::sr25519_sign
     */
    runtime::WasmSpan ext_sr25519_sign_version_1(runtime::WasmSize key_type,
                                          runtime::WasmPointer key,
                                          runtime::WasmSpan msg);

    /**
     * @see Extension::ext_sr25519_verify
     */
    runtime::WasmSize ext_sr25519_verify_version_1(runtime::WasmPointer sig,
                                            runtime::WasmSpan msg,
                                            runtime::WasmPointer pubkey_data);

    /**
     * @see Extension::ext_crypto_secp256k1_ecdsa_recover_version_1
     */
    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_version_1(
        runtime::WasmPointer sig, runtime::WasmPointer msg);

    /**
     * @see Extension::ext_crypto_secp256k1_ecdsa_recover_compressed_version_1
     */
    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
        runtime::WasmPointer sig, runtime::WasmPointer msg);

   private:
    common::Blob<32> deriveSeed(std::string_view content);

    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
    std::shared_ptr<crypto::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::CryptoStore> crypto_store_;
    std::shared_ptr<crypto::Bip39Provider> bip39_provider_;
    boost::optional<std::queue<std::future<runtime::WasmSize>>> batch_verify_;
    log::Logger logger_;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_CRYPTO_EXTENSION_HPP
