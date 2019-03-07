/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_EXTENSION_HPP
#define KAGOME_STORAGE_EXTENSION_HPP

namespace extensions {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtension {
   public:
    uint8_t *ext_child_storage_root(const uint8_t *storage_key_data,
                                    uint32_t storage_key_length,
                                    uint32_t *written) {
      std::terminate();
    }
    void ext_clear_child_storage(const uint8_t *storage_key_data,
                                 uint32_t storage_key_length,
                                 const uint8_t *key_data,
                                 uint32_t key_length) {
      std::terminate();
    }
    void ext_clear_prefix(const uint8_t *prefix_data, uint32_t prefix_length) {
      std::terminate();
    }
    void ext_clear_storage(const uint8_t *key_data, uint32_t key_length) {
      std::terminate();
    }
    uint32_t ext_exists_child_storage(const uint8_t *storage_key_data,
                                      uint32_t storage_key_length,
                                      const uint8_t *key_data,
                                      uint32_t key_length) {
      std::terminate();
    }
    uint8_t *ext_get_allocated_child_storage(const uint8_t *storage_key_data,
                                             uint32_t storage_key_length,
                                             const uint8_t *key_data,
                                             uint32_t key_length,
                                             uint32_t *written) {
      std::terminate();
    }
    uint8_t *ext_get_allocated_storage(const uint8_t *key_data,
                                       uint32_t key_length,
                                       uint32_t *written) {
      std::terminate();
    }
    uint32_t ext_get_child_storage_into(const uint8_t *storage_key_data,
                                        uint32_t storage_key_length,
                                        const uint8_t *key_data,
                                        uint32_t key_length,
                                        uint8_t *value_data,
                                        uint32_t value_length,
                                        uint32_t value_offset) {
      std::terminate();
    }
    uint32_t ext_get_storage_into(const uint8_t *key_data,
                                  uint32_t key_length,
                                  uint8_t *value_data,
                                  uint32_t value_length,
                                  uint32_t value_offset) {
      std::terminate();
    }
    void ext_kill_child_storage(const uint8_t *storage_key_data,
                                uint32_t storage_key_length) {
      std::terminate();
    }
    void ext_set_child_storage(const uint8_t *storage_key_data,
                               uint32_t storage_key_length,
                               const uint8_t *key_data,
                               uint32_t key_length,
                               uint8_t *value_data,
                               uint32_t value_length) {
      std::terminate();
    }
    void ext_set_storage(const uint8_t *key_data,
                         uint32_t key_length,
                         uint8_t *value_data,
                         uint32_t value_length) {
      std::terminate();
    }
    uint32_t ext_storage_changes_root(const uint8_t *parent_hash_data,
                                      uint32_t parent_hash_len,
                                      uint64_t parent_num,
                                      uint8_t *result) {
      std::terminate();
    }
    void ext_storage_root(uint8_t *result) {
      std::terminate();
    }
    uint32_t ext_exists_storage(const uint8_t *key_data, uint32_t key_length) {
      std::terminate();
    }
  };
}  // namespace extensions

#endif  // KAGOME_STORAGE_EXTENSION_HPP
