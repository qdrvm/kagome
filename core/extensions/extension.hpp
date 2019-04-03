/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSION_HPP
#define KAGOME_EXTENSION_HPP

#include <cstdint>
#include <functional>

#include "runtime/wasm_memory.hpp"

namespace kagome::extensions {
  /**
   * Extensions for WASM; API, which is called by the runtime to control RE
   */
  class Extension {
   protected:
    using SandoxDispatchFuncType = std::function<uint64_t(
        const uint8_t *serialized_args, size_t serialized_args_length,
        size_t state, size_t func_index)>;

   public:
    // -------------------------Storage extensions--------------------------

    /**
     * @brief Deletes values by keys containing given prefix
     * @param prefix_data pointer to the prefix
     * @param prefix_length lemgth of the prefix
     */
    virtual void ext_clear_prefix(runtime::WasmPointer prefix_data,
                                  runtime::SizeType prefix_length) = 0;

    /**
     * @brief Deletes value by given key
     * @param key_data
     * @param key_length
     */
    virtual void ext_clear_storage(runtime::WasmPointer key_data,
                                   runtime::SizeType key_length) = 0;

    /**
     * @brief Checks if the given key exists in the storage.
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @return 1 if value exists, 0 otherwise
     */
    virtual runtime::SizeType ext_exists_storage(
        runtime::WasmPointer key_data, runtime::SizeType key_length) = 0;

    /**
     * Gets the value of the given key from storage, allocates memory for that
     * value, stores value in that memory and returns pointer to it
     *
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param len_ptr pointer to where length of the value is stored
     * @return - 0 if no value exists to the given key. written_out is set
     * to u32::max_value().
     * - Otherwise, pointer to the value in memory. written_out contains the
     * length of the value.
     */
    virtual runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data, runtime::SizeType key_length,
        runtime::WasmPointer len_ptr) = 0;

    /**
     * Gets the value of the given key from storage. The value is written into
     * value starting at value_offset. If the value length is greater than
     * value_len - value_offset, the value is written partially.
     *
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param value_data pointer where to store result
     * @param value_length max length of the data stored at value_data ptr
     * @param value_offset offset starting from which value associated with key
     * is obtained
     * @return u32::max_value() if the value does not
     * exist. Otherwise, the number of bytes written for value.
     */
    virtual runtime::SizeType ext_get_storage_into(
        runtime::WasmPointer key_data, runtime::SizeType key_length,
        runtime::WasmPointer value_data, runtime::SizeType value_length,
        runtime::SizeType value_offset) = 0;

    /**
     * @brief Sets the value by given key
     * @param key_data pointer to the key
     * @param key_length length of the key
     * @param value_data pointer to the value
     * @param value_length length of the value
     */
    virtual void ext_set_storage(runtime::WasmPointer key_data,
                                 runtime::SizeType key_length,
                                 runtime::WasmPointer value_data,
                                 runtime::SizeType value_length) = 0;

    /**
     * Calculate ordered trie root from provided values
     * @param values_data pointer to array of values to calculate hash
     * @param lens_data pointer to the array of lengths for values
     * @param lens_length size of lengths array
     * @param result pointer to store trie root
     */
    virtual void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data, runtime::WasmPointer lens_data,
        runtime::SizeType lens_length, runtime::WasmPointer result) = 0;
    /**
     * @brief Get the change trie root of the current storage overlay at a block
     * with given parent.
     *
     * @param parent_hash_data pointer to the hash of parent block
     * @param parent_hash_len length of parent block
     * @param parent_num number of parent block
     * @param result pointer to place change trie root
     * @return 1 if change trie root was found, 0 otherwise
     */
    virtual runtime::SizeType ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::SizeType parent_hash_len, runtime::SizeType parent_num,
        runtime::WasmPointer result) = 0;

    /**
     * @brief Gets the trie root of the storage
     *
     * @param result is the pointer where the root will be written
     */
    virtual void ext_storage_root(runtime::WasmPointer result) const = 0;

    /**
     * A child storage function
     * @see ext_storage_root for details
     */
    virtual runtime::WasmPointer ext_child_storage_root(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length,
        runtime::WasmPointer written) = 0;

    /**
     * A child storage function
     * @see ext_clear_storage for details
     */
    virtual void ext_clear_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length) = 0;

    /**
     * A child storage function
     * @see ext_exists_storage for details
     */
    virtual runtime::SizeType ext_exists_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length) = 0;

    /**
     * A child storage function
     * @see ext_get_allocated_storage for details
     */
    virtual runtime::WasmPointer ext_get_allocated_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer written) = 0;

    /**
     * A child storage function
     * @see ext_get_storage_into for details
     */
    virtual runtime::SizeType ext_get_child_storage_into(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer value_data,
        runtime::SizeType value_length, runtime::SizeType value_offset) = 0;

    /**
     * @brief removes child storage by give storage key
     * @param storage_key_data pointer to storage key
     * @param storage_key_length length of storage key
     */
    virtual void ext_kill_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::SizeType storage_key_length) = 0;

    /**
     * A child storage function
     * @see ext_set_storage for details
     */
    virtual void ext_set_child_storage(runtime::WasmPointer storage_key_data,
                                       runtime::SizeType storage_key_length,
                                       runtime::WasmPointer key_data,
                                       runtime::SizeType key_length,
                                       runtime::WasmPointer value_data,
                                       runtime::SizeType value_length) = 0;

    // -------------------------Memory extensions--------------------------
    /**
     * allocate wasm memory of given size returning a pointer to the beginning
     * of allocated memory chunk
     * @param size number of bytes to allocate
     * @return pointer to the beginning of allocated memory chunk. If memory
     * cannot be allocated then return -1
     */
    virtual runtime::WasmPointer ext_malloc(runtime::SizeType size) = 0;

    /**
     * Deallocate the space previously allocated by ext_malloc
     * @param ptr pointer to the memory to deallocate
     */
    virtual void ext_free(runtime::WasmPointer ptr) = 0;

    // -------------------------I/O extensions--------------------------

    /**
     * Print a hex value
     * @param data - pointer to an array of bytes with hex
     * @param length of the array
     */
    virtual void ext_print_hex(const uint8_t *data, uint32_t length) = 0;

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
    virtual void ext_print_utf8(const uint8_t *utf8_data,
                                uint32_t utf8_length) = 0;

    // -------------------------Cryptographic extensions------------------------

    /**
     * Hash the data using blake2b hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_blake2_256(const uint8_t *data, uint32_t len,
                                uint8_t *out) = 0;

    /**
     * Verify the signature over the ed25519 message
     * @param msg_data - msg to be verified
     * @param msg_len - length of the msg
     * @param sig_data - signature of the message
     * @param pubkey_data - key of possible message's author
     * @return 0, if key is successfully verified, 5 otherwise
     */
    virtual uint32_t ext_ed25519_verify(const uint8_t *msg_data,
                                        uint32_t msg_len,
                                        const uint8_t *sig_data,
                                        const uint8_t *pubkey_data) = 0;

    /**
     * Hash the data using XX128 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_128(const uint8_t *data, uint32_t len,
                              uint8_t *out) = 0;

    /**
     * Hash the data using XX256 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_256(const uint8_t *data, uint32_t len,
                              uint8_t *out) = 0;

    // -------------------------Sanboxing extensions--------------------------

    virtual void ext_sandbox_instance_teardown(uint32_t instance_idx) = 0;

    virtual uint32_t ext_sandbox_instantiate(
        const SandoxDispatchFuncType &dispatch_func, const uint8_t *wasm_ptr,
        size_t wasm_length, const uint8_t *imports_ptr, size_t imports_length,
        size_t state) = 0;

    virtual uint32_t ext_sandbox_invoke(
        uint32_t instance_idx, const uint8_t *export_ptr, size_t export_len,
        const uint8_t *args_ptr, size_t args_len, uint8_t *return_val_ptr,
        size_t return_val_len, size_t state) = 0;

    virtual uint32_t ext_sandbox_memory_get(uint32_t memory_idx,
                                            uint32_t offset, uint8_t *buf_ptr,
                                            size_t buf_length) = 0;

    virtual uint32_t ext_sandbox_memory_new(uint32_t initial,
                                            uint32_t maximum) = 0;

    virtual uint32_t ext_sandbox_memory_set(uint32_t memory_idx,
                                            uint32_t offset,
                                            const uint8_t *val_ptr,
                                            size_t val_len) = 0;

    virtual void ext_sandbox_memory_teardown(uint32_t memory_idx) = 0;

    // -------------------------Misc extensions--------------------------
    virtual uint64_t ext_chain_id() = 0;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSION_HPP
