/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_API_HPP
#define KAGOME_HOST_API_HPP

#include <cstdint>
#include <functional>

#include "runtime/types.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/wasm_result.hpp"

namespace kagome::host_api {
  /**
   * Extensions for WASM; API, which is called by the runtime to control RE
   */
  class HostApi {
   public:
    virtual ~HostApi() = default;

    virtual std::shared_ptr<runtime::WasmMemory> memory() const = 0;
    virtual void reset() = 0;

    // -------------------------Storage extensions--------------------------

    /**
     * @brief Deletes values by keys containing given prefix
     * @param prefix_data pointer to the prefix
     * @param prefix_length length of the prefix
     */
    virtual void ext_clear_prefix(runtime::WasmPointer prefix_data,
                                  runtime::WasmSize prefix_length) = 0;

    /**
     * @brief Deletes value by given key
     * @param key_data pointer to the key
     * @param key_length length of the key
     */
    virtual void ext_clear_storage(runtime::WasmPointer key_data,
                                   runtime::WasmSize key_length) = 0;

    /**
     * @brief Checks if the given key exists in the storage.
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @return 1 if value exists, 0 otherwise
     */
    [[nodiscard]] virtual runtime::WasmSize ext_exists_storage(
        runtime::WasmPointer key_data, runtime::WasmSize key_length) const = 0;

    /**
     * Gets the value of the given key from storage, allocates memory for that
     * value, stores value in that memory and returns pointer to it
     *
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param len_ptr pointer to where length of the value is stored. Set to
     * u32::max_value() if no value is found for a key
     * @return
     * <li>value found => pointer to the value
     * <li>no value for a given key => 0
     * <li>there is no enough memory to allocate a value => -1
     */
    [[nodiscard]] virtual runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer len_ptr) = 0;

    /**
     * Gets the value of the given key from storage. Part of the value starting
     * at the value_offset is written into value_data ptr. If the value length
     * is greater than value_len - value_offset, the value is written partially.
     *
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param value_data pointer where to store result
     * @param value_length max length of the data that can be stored at
     * value_data ptr
     * @param value_offset offset starting from which value associated with key
     * is obtained
     * @return
     * <li>value is found => the number of bytes written for value.
     * <li>value is not found => u32::max_value()
     */
    [[nodiscard]] virtual runtime::WasmSize ext_get_storage_into(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer value_data,
        runtime::WasmSize value_length,
        runtime::WasmSize value_offset) = 0;

    /**
     * @brief Sets the value by given key
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param value_data pointer to the value
     * @param value_length length of the value
     */
    virtual void ext_set_storage(runtime::WasmPointer key_data,
                                 runtime::WasmSize key_length,
                                 runtime::WasmPointer value_data,
                                 runtime::WasmSize value_length) = 0;

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
     * Calculate ordered trie root from provided values
     * @param values_data pointer to array of values to calculate hash
     * @param lens_data pointer to the array of lengths for values
     * @param lens_length size of lengths array
     * @param result pointer to store trie root
     */
    virtual void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data,
        runtime::WasmPointer lens_data,
        runtime::WasmSize lens_length,
        runtime::WasmPointer result) = 0;

    /**
     * @brief Get the change trie root of the current storage overlay at a block
     * with given parent.
     *
     * @param parent_hash_data pointer to the hash of parent block
     * @param result pointer to place change trie root
     * @return 1 if change trie root was found, 0 otherwise
     */
    [[nodiscard]] virtual runtime::WasmSize ext_storage_changes_root(
        runtime::WasmPointer parent_hash, runtime::WasmPointer result) = 0;

    /**
     * @brief Gets the trie root of the storage
     *
     * @param result is the pointer where the root will be written
     */
    virtual void ext_storage_root(runtime::WasmPointer result) const = 0;

    /**
     * @brief Starts new (possible is nested) transaction
     */
    virtual void ext_storage_start_transaction() = 0;

    /**
     * @brief Rollback last started transaction
     */
    virtual void ext_storage_rollback_transaction() = 0;

    /**
     * @brief Commit last started transaction
     */
    virtual void ext_storage_commit_transaction() = 0;

    // ------------------------ Storage extensions v1 ------------------------

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
     * @brief Clear the storage of each key/value pair where the key starts with
     * the given prefix.
     * @param prefix memory span containing prefix
     * @param limit of entries to be removed
     */
    virtual runtime::WasmSpan ext_storage_clear_prefix_version_2(
        runtime::WasmSpan prefix, runtime::WasmSpan limit) = 0;

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

    // -------------------------Memory extensions--------------------------
    /**
     * allocate wasm memory of given size returning a pointer to the beginning
     * of allocated memory chunk
     * @param size number of bytes to allocate
     * @return pointer to the beginning of allocated memory chunk. If memory
     * cannot be allocated then return -1
     */
    [[nodiscard]] virtual runtime::WasmPointer ext_malloc(
        runtime::WasmSize size) = 0;

    /**
     * Deallocate the space previously allocated by ext_malloc
     * @param ptr pointer to the memory to deallocate
     */
    virtual void ext_free(runtime::WasmPointer ptr) = 0;

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

    /**
     * Print a hex value
     * @param data - pointer to an array of bytes with hex
     * @param length of the array
     */
    virtual void ext_print_hex(runtime::WasmPointer data,
                               runtime::WasmSize length) = 0;

    /**
     * Print a number
     * @param value - number to be printed
     */
    virtual void ext_print_num(uint64_t value) = 0;

    /**
     * Print a UTF-8-encoded string
     * @param utf8_data - pointer to an array of bytes with UTF-8
     * @param utf8_length - length of the array
     */
    virtual void ext_print_utf8(runtime::WasmPointer utf8_data,
                                runtime::WasmSize utf8_length) = 0;

    // -------------------------Cryptographic extensions------------------------

    /**
     * Hash the data using blake2b hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_blake2_128(runtime::WasmPointer data,
                                runtime::WasmSize len,
                                runtime::WasmPointer out) = 0;

    /**
     * Hash the data using blake2b hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_blake2_256(runtime::WasmPointer data,
                                runtime::WasmSize len,
                                runtime::WasmPointer out) = 0;

    /**
     * Hash the data using keccak hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_keccak_256(runtime::WasmPointer data,
                                runtime::WasmSize len,
                                runtime::WasmPointer out) = 0;

    /**
     * @brief Starts the verification extension. The extension is a separate
     * background process and is used to parallel-verify signatures which are
     * pushed to the batch with ext_crypto_.._verify
     */
    virtual void ext_start_batch_verify() = 0;

    /**
     * @brief Finish verifying the batch of signatures since the last call to
     * this function. Blocks until all the signatures are verified.
     * @throws runtime_error if no verification extension is registered
     * (ext_crypto_start_batch_verify (E.3.15) tchwas not called.)
     * @returns an i32 integer value equal to 1 if all the signatures are valid
     * or a value equal to 0 if one or more of the signatures are invalid.
     */
    [[nodiscard]] virtual runtime::WasmSize ext_finish_batch_verify() = 0;

    /**
     * Verify the signature over the ed25519 message
     * @param msg_data - msg to be verified
     * @param msg_len - length of the msg
     * @param sig_data - signature of the message
     * @param pubkey_data - key of possible message's author
     * @return 0, if key is successfully verified, 5 otherwise
     */
    [[nodiscard]] virtual runtime::WasmSize ext_ed25519_verify(
        runtime::WasmPointer msg_data,
        runtime::WasmSize msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) = 0;

    /**
     * Verify the signature over the sr25519 message
     * @param msg_data - msg to be verified
     * @param msg_len - length of the msg
     * @param sig_data - signature of the message
     * @param pubkey_data - key of possible message's author
     * @return 0, if key is successfully verified, 5 otherwise
     */
    [[nodiscard]] virtual runtime::WasmSize ext_sr25519_verify(
        runtime::WasmPointer msg_data,
        runtime::WasmSize msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) = 0;

    /**
     * Hash the data using XX64 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_64(runtime::WasmPointer data,
                             runtime::WasmSize len,
                             runtime::WasmPointer out) = 0;

    /**
     * Hash the data using XX128 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_128(runtime::WasmPointer data,
                              runtime::WasmSize len,
                              runtime::WasmPointer out) = 0;

    /**
     * Hash the data using XX256 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_256(runtime::WasmPointer data,
                              runtime::WasmSize len,
                              runtime::WasmPointer out) = 0;
    // crypto v1

    /**
     * Recover secp256k1 public key
     * @param sig recoverable 65-byte signature
     * @param msg blake2s message hash
     * @return pointer-size value (pointer to buffer and its size) containing
     * scale-encoded variant of public key or error
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_secp256k1_ecdsa_recover_v1(runtime::WasmPointer sig,
                                          runtime::WasmPointer msg) = 0;

    /**
     * Recover secp256k1 public key
     * @param sig recoverable 65-byte signature
     * @param msg blake2s message hash
     * @return pointer-size value (pointer to buffer and its size) containing
     * scale-encoded variant of compressed public key or error
     */
    [[nodiscard]] virtual runtime::WasmSpan
    ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
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
    [[nodiscard]] virtual runtime::WasmSpan ext_ed25519_public_keys_v1(
        runtime::WasmSize key_type) = 0;

    /**
     * @see Extension::ext_ed25519_generate
     */
    [[nodiscard]] virtual runtime::WasmPointer ext_ed25519_generate_v1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) = 0;

    /**
     * @see Extension::ext_ed25519_sign
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_ed25519_sign_v1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) = 0;

    /**
     * @see Extension::ext_ed25519_verify
     */
    [[nodiscard]] virtual runtime::WasmSize ext_ed25519_verify_v1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) = 0;

    /**
     * @see Extension::ext_sr25519_public_keys
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_sr25519_public_keys_v1(
        runtime::WasmSize key_type) = 0;

    /**
     * @see Extension::ext_sr25519_generate
     */
    [[nodiscard]] virtual runtime::WasmPointer ext_sr25519_generate_v1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) = 0;

    /**
     * @see Extension::ext_sr25519_sign
     */
    [[nodiscard]] virtual runtime::WasmSpan ext_sr25519_sign_v1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) = 0;

    /**
     * @see Extension::ext_sr25519_verify
     */
    [[nodiscard]] virtual runtime::WasmSize ext_sr25519_verify_v1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) = 0;

    // -------------------------Misc extensions--------------------------
    [[nodiscard]] virtual uint64_t ext_chain_id() const = 0;

    [[nodiscard]] virtual runtime::WasmResult
    ext_misc_runtime_version_version_1(runtime::WasmSpan data) const = 0;

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
