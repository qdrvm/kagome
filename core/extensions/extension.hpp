/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSION_HPP
#define KAGOME_EXTENSION_HPP

#include <cstdint>
#include <functional>

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
    /// storage extensions
    virtual uint8_t *ext_child_storage_root(const uint8_t *storage_key_data,
                                            uint32_t storage_key_length,
                                            uint32_t *written) = 0;

    virtual void ext_clear_child_storage(const uint8_t *storage_key_data,
                                         uint32_t storage_key_length,
                                         const uint8_t *key_data,
                                         uint32_t key_length) = 0;

    virtual void ext_clear_prefix(const uint8_t *prefix_data,
                                  uint32_t prefix_length) = 0;

    virtual void ext_clear_storage(const uint8_t *key_data,
                                   uint32_t key_length) = 0;

    virtual uint32_t ext_exists_child_storage(const uint8_t *storage_key_data,
                                              uint32_t storage_key_length,
                                              const uint8_t *key_data,
                                              uint32_t key_length) = 0;

    virtual uint8_t *ext_get_allocated_child_storage(
        const uint8_t *storage_key_data, uint32_t storage_key_length,
        const uint8_t *key_data, uint32_t key_length, uint32_t *written) = 0;

    virtual uint8_t *ext_get_allocated_storage(const uint8_t *key_data,
                                               uint32_t key_length,
                                               uint32_t *written) = 0;

    virtual uint32_t ext_get_child_storage_into(
        const uint8_t *storage_key_data, uint32_t storage_key_length,
        const uint8_t *key_data, uint32_t key_length, uint8_t *value_data,
        uint32_t value_length, uint32_t value_offset) = 0;

    virtual uint32_t ext_get_storage_into(const uint8_t *key_data,
                                          uint32_t key_length,
                                          uint8_t *value_data,
                                          uint32_t value_length,
                                          uint32_t value_offset) = 0;

    virtual void ext_kill_child_storage(const uint8_t *storage_key_data,
                                        uint32_t storage_key_length) = 0;

    virtual void ext_set_child_storage(const uint8_t *storage_key_data,
                                       uint32_t storage_key_length,
                                       const uint8_t *key_data,
                                       uint32_t key_length,
                                       const uint8_t *value_data,
                                       uint32_t value_length) = 0;

    virtual void ext_set_storage(const uint8_t *key_data, uint32_t key_length,
                                 const uint8_t *value_data,
                                 uint32_t value_length) = 0;

    virtual uint32_t ext_storage_changes_root(const uint8_t *parent_hash_data,
                                              uint32_t parent_hash_len,
                                              uint64_t parent_num,
                                              uint8_t *result) = 0;

    virtual void ext_storage_root(uint8_t *result) = 0;

    virtual uint32_t ext_exists_storage(const uint8_t *key_data,
                                        uint32_t key_length) = 0;

    /// memory extensions
    /**
     * allocate wasm memory of given size returning a pointer to the beginning
     * of allocated memory chunk
     * @param size number of bytes to allocate
     * @return pointer to the beginning of allocated memory chunk. If memory
     * cannot be allocated then return -1
     */
    virtual int32_t ext_malloc(uint32_t size) = 0;

    /**
     * Deallocate the space previously allocated by ext_malloc
     * @param ptr pointer to the memory to deallocate
     */
    virtual void ext_free(int32_t ptr) = 0;

    /// I/O extensions

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

    /// cryptographic extensions

    /**
     * Hash the data using blake2b hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_blake2_256(const uint8_t *data, uint32_t len,
                                uint8_t *out) = 0;

    /**
     * Create a trie root from enumerated values
     * @note NOT IMPLEMENTED
     */
    virtual void ext_blake2_256_enumerated_trie_root(const uint8_t *values_data,
                                                     const uint32_t *lens_data,
                                                     uint32_t lens_length,
                                                     uint8_t *result) = 0;

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

    /// sandboxing extensions
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

    /// misc extensions
    virtual uint64_t ext_chain_id() = 0;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSION_HPP
