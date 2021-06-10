/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_API_HPP
#define KAGOME_HOST_API_HPP

#include <cstdint>
#include <functional>
#include <memory>

#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class Memory;
}

namespace kagome::host_api {
  /**
   * Extensions for the runtime wasm module, which are called by the runtime to
   * access the host functionality
   */
  class HostApi {
   public:
    virtual ~HostApi() = default;

    virtual void reset() = 0;

    // ------------------------ Storage extensions v1 ------------------------

    /**
     * @brief Reads data from storage with the given key
     * @param key pointer-size to the key
     * @param value_out pointer-size to the read data
     * @param offset in bytes from the data block begin should be read
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_storage_read_version_1(
        runtime::WasmSpan key,
        runtime::WasmSpan value_out,
        runtime::WasmOffset offset) = 0;

    /**
     * @brief Sets the value under a given key into storage.
     * @param key memory span containing key
     * @param value memory span containing value
     */
    virtual void ext_storage_set_version_1(runtime::WasmSpan key,
                                           runtime::WasmSpan value) = 0;

    /**
     * @brief Retrieves the value associated with the given key from storage.
     * @param key key memory span containing key
     * @return value memory span containing scale-encoded optional value
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_storage_get_version_1(
        runtime::WasmSpan key) = 0;

    /**
     * @brief Clears the storage of the given key and its value.
     * @param key_data memory span containing key
     */
    virtual void ext_storage_clear_version_1(runtime::WasmSpan key_data) = 0;

    /**
     * @brief Checks whether the given key exists in storage.
     * @param key_data memory span containing key
     * @return an i32 integer value equal to 1 if the key exists or a value
     * equal to 0 if otherwise.
     */
    [[nodiscard]] virtual runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const = 0;

    /**
     * @brief Clear the storage of each key/value pair where the key starts with
     * the given prefix.
     * @param prefix memory span containing prefix
     */
    virtual void ext_storage_clear_prefix_version_1(
        runtime::WasmSpan prefix) = 0;

    /**
     * @brief Commits all existing operations and computes the resulting storage
     * root.
     * @return memory span containing scale-encoded storage root
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_storage_root_version_1() = 0;

    /**
     * Commits all existing operations and gets the resulting change
     * root. The parent hash is a SCALE encoded change root.
     * @param parent_hash wasm span containing parent hash
     * @return wasm span containing scale-encoded optional change root
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash) = 0;

    /**
     * Gets the next key in storage after the given one in lexicographic order.
     * @param key memory span containing key
     * @return wasm span containing scale-encoded optional next key
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const = 0;

    /**
     * Appends the scale encoded value to the scale encoded storage item at the
     * given key
     * @param key memory span containing key
     * @param value memory span containing value that should be appended to
     * storage item under the @param key
     *
     * @note This assumes specific format of the storage item. Also there is no
     * way to undo this operation.
     */
    virtual void ext_storage_append_version_1(
        runtime::WasmSpan key, runtime::WasmSpan value) const = 0;

    /**
     * Conducts a 256-bit Blake2 trie root formed from the iterated items.
     * @param values_data wasm span containing the iterated items from which the
     * trie root gets formed. The items consist of a SCALE encoded array
     * containing arbitrary key/value pairs.
     * @return wasm span containing the 256-bit trie root result
     */
    [[nodiscard]] virtual runtime::WasmPointer
    ext_trie_blake2_256_root_version_1(runtime::WasmSpan values_data) = 0;

    /**
     * Conducts a 256-bit Blake2 trie root formed from the enumerated items.
     * @param values_data wasm span containing the enumerated items from which
     * the trie root gets formed. The items consist of a SCALE encoded array
     * containing only values, where the corre-sponding key of each value is the
     * index of the item in the array, starting at 0. The keys are
     * little-endian, fixed-size integers.
     * @return wasm span containing the 256-bit trie root result
     */
    [[nodiscard]] virtual runtime::WasmPointer
    ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data) = 0;

    // ------------------------Memory extensions v1-------------------------
    /**
     * @see Extension::ext_malloc
     */
    [[nodiscard]] virtual runtime::WasmPointer ext_allocator_malloc_version_1(
        runtime::WasmSize size) = 0;

    /**
     * @see  Extension::ext_free
     */
    virtual void ext_allocator_free_version_1(runtime::WasmPointer ptr) = 0;

    // -------------------------I/O extensions--------------------------

    /**
     * Print a log message
     * @param level - log level of the message
     * @param target pointer-size value of the message source
     * @param message pointer-size value of the message content
     */
    virtual void ext_logging_log_version_1(runtime::WasmEnum level,
                                           runtime::WasmSpan target,
                                           runtime::WasmSpan message) = 0;
    /**
     * Get host max log level
     */
    virtual runtime::WasmEnum ext_logging_max_level_version_1() = 0;

    // -------------------------Cryptographic extensions------------------------

    /**
     * @brief Starts the verification extension. The extension is a separate
     * background process and is used to parallel-verify signatures which are
     * pushed to the batch with ext_crypto_.._verify
     */
    virtual void ext_crypto_start_batch_verify_version_1() = 0;

    /**
     * @brief Finish verifying the batch of signatures since the last call to
     * this function. Blocks until all the signatures are verified.
     * @throws runtime_error if no verification extension is registered
     * (ext_crypto_start_batch_verify (E.3.15) tchwas not called.)
     * @returns an i32 integer value equal to 1 if all the signatures are valid
     * or a value equal to 0 if one or more of the signatures are invalid.
     */
    [[nodiscard]] virtual int32_t
    ext_crypto_finish_batch_verify_version_1() = 0;

    /**
     * Recover secp256k1 public key
     * @param sig recoverable 65-byte signature
     * @param msg blake2s message hash
     * @return pointer-size value (pointer to buffer and its size) containing
     * scale-encoded variant of public key or error
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_secp256k1_ecdsa_recover_version_1(runtime::WasmPointer sig,
                                                 runtime::WasmPointer msg) = 0;

    /**
     * Recover secp256k1 public key
     * @param sig recoverable 65-byte signature
     * @param msg blake2s message hash
     * @return pointer-size value (pointer to buffer and its size) containing
     * scale-encoded variant of compressed public key or error
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
        runtime::WasmPointer sig, runtime::WasmPointer msg) = 0;

    // ------------------------- Hashing extension/crypto ---------------

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_keccak_256_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_sha2_256_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_blake2_128_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_blake2_256_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_twox_64_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_twox_128_version_1(
        runtime::WasmSpan data) = 0;

    [[nodiscard]] virtual runtime::WasmPointer ext_hashing_twox_256_version_1(
        runtime::WasmSpan data) = 0;

    // -------------------------Crypto extensions v1---------------------

    /**
     * @see Extension::ext_ed25519_public_keys
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_ed25519_public_keys_version_1(runtime::WasmSize key_type) = 0;

    /**
     * @see Extension::ext_ed25519_generate
     */
    [[nodiscard]] virtual runtime::WasmPointer
    ext_crypto_ed25519_generate_version_1(runtime::WasmSize key_type,
                                          runtime::WasmSpan seed) = 0;

    /**
     * @see Extension::ext_ed25519_sign
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_crypto_ed25519_sign_version_1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) = 0;

    /**
     * @see Extension::ext_ed25519_verify
     */
    [[nodiscard]] virtual runtime::WasmSize ext_crypto_ed25519_verify_version_1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) = 0;

    /**
     * @see Extension::ext_sr25519_public_keys
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_sr25519_public_keys_version_1(runtime::WasmSize key_type) = 0;

    /**
     * @see Extension::ext_sr25519_generate
     */
    [[nodiscard]] virtual runtime::WasmPointer
    ext_crypto_sr25519_generate_version_1(runtime::WasmSize key_type,
                                          runtime::WasmSpan seed) = 0;

    /**
     * @see Extension::ext_sr25519_sign
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_crypto_sr25519_sign_version_1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) = 0;

    /**
     * Verifies an sr25519 signature. Only version 1 of this function supports
     * deprecated Schnorr signatures introduced by the schnorrkel Rust library
     * version 0.1.1 and should only be used for backward compatibility.
     * Returns true when the verification is either successful or batched.
     * If no batching verification
     * extension is registered, this function will fully verify the signature
     * and return the result. If batching verification is registered, this
     * function will push the data to the batch and return immediately. The
     * caller can then get the result by calling ext_crypto_finish_batch_verify
     * The verification extension is explained more in detail in
     * ext_crypto_start_batch_verify
     */
    [[nodiscard]] virtual int32_t ext_crypto_sr25519_verify_version_1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) = 0;

    [[nodiscard]] virtual int32_t ext_crypto_sr25519_verify_version_2(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) = 0;

    // -------------------------Misc extensions--------------------------

    [[nodiscard]] virtual runtime::WasmSpan ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const = 0;

    /**
     * Print a hex value
     * @param data pointer-size to an array of bytes with hex
     */
    virtual void ext_misc_print_hex_version_1(runtime::WasmSpan data) const = 0;

    /**
     * Print a number
     * @param value - number to be printed
     */
    virtual void ext_misc_print_num_version_1(uint64_t value) const = 0;

    /**
     * Print a UTF-8-encoded string
     * @param data pointer-size to an array of bytes with UTF-8
     */
    virtual void ext_misc_print_utf8_version_1(
        runtime::WasmSpan data) const = 0;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_HOST_API_HPP
