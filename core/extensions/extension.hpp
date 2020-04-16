/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_EXTENSION_HPP
#define KAGOME_EXTENSION_HPP

#include <cstdint>
#include <functional>

#include "runtime/types.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::extensions {
  /**
   * Extensions for WASM; API, which is called by the runtime to control RE
   */
  class Extension {
   public:
    virtual ~Extension() = default;

    virtual std::shared_ptr<runtime::WasmMemory> memory() const = 0;
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
     * @param key_data pointer to the key
     * @param key_length length of the key
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
        runtime::WasmPointer key_data, runtime::SizeType key_length) const = 0;

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
    virtual runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length,
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
    virtual runtime::SizeType ext_get_storage_into(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length,
        runtime::WasmPointer value_data,
        runtime::SizeType value_length,
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
        runtime::WasmPointer values_data,
        runtime::WasmPointer lens_data,
        runtime::SizeType lens_length,
        runtime::WasmPointer result) = 0;

    /**
     * @brief Get the change trie root of the current storage overlay at a block
     * with given parent.
     *
     * @param parent_hash_data pointer to the hash of parent block
     * @param parent_hash_len length of parent block
     * @param result pointer to place change trie root
     * @return 1 if change trie root was found, 0 otherwise
     */
    virtual runtime::SizeType ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::SizeType parent_hash_len,
        runtime::WasmPointer result) = 0;

    /**
     * @brief Gets the trie root of the storage
     *
     * @param result is the pointer where the root will be written
     */
    virtual void ext_storage_root(runtime::WasmPointer result) const = 0;

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
    virtual void ext_print_hex(runtime::WasmPointer data,
                               runtime::SizeType length) = 0;

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
                                runtime::SizeType utf8_length) = 0;

    // -------------------------Cryptographic extensions------------------------

    /**
     * Hash the data using blake2b hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_blake2_256(runtime::WasmPointer data,
                                runtime::SizeType len,
                                runtime::WasmPointer out) = 0;

    /**
     * Hash the data using keccak hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_keccak_256(runtime::WasmPointer data,
                                runtime::SizeType len,
                                runtime::WasmPointer out) = 0;

    /**
     * Verify the signature over the ed25519 message
     * @param msg_data - msg to be verified
     * @param msg_len - length of the msg
     * @param sig_data - signature of the message
     * @param pubkey_data - key of possible message's author
     * @return 0, if key is successfully verified, 5 otherwise
     */
    virtual runtime::SizeType ext_ed25519_verify(
        runtime::WasmPointer msg_data,
        runtime::SizeType msg_len,
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
    virtual runtime::SizeType ext_sr25519_verify(
        runtime::WasmPointer msg_data,
        runtime::SizeType msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) = 0;

    /**
     * Hash the data using XX64 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_64(runtime::WasmPointer data,
                             runtime::SizeType len,
                             runtime::WasmPointer out) = 0;

    /**
     * Hash the data using XX128 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_128(runtime::WasmPointer data,
                              runtime::SizeType len,
                              runtime::WasmPointer out) = 0;

    /**
     * Hash the data using XX256 hash
     * @param data to be hashed
     * @param len of the data
     * @param out buffer to store the hash
     */
    virtual void ext_twox_256(runtime::WasmPointer data,
                              runtime::SizeType len,
                              runtime::WasmPointer out) = 0;

    // -------------------------Misc extensions--------------------------
    virtual uint64_t ext_chain_id() const = 0;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSION_HPP
