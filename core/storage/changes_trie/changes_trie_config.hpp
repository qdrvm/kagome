/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG
#define KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG

#include <cstdint>

namespace kagome::storage::changes_trie {

  struct ChangesTrieConfig {
    /**
     * The interval (in blocks) at which
     * block mappings are created. Block mappings are not created when this is
     * less or equal to 1.
     */
    uint32_t digest_interval;
    
    /**
     * Maximal number of levels in the
     * hierarchy. 0 means that block mappings are not created at
     * all. 1 means only the regular digest_interval block
     * mappings are created. Any other level means that the block mappings are
     * created every (digest_interval in power of digest_levels) block for
     * each level in 1 to digest_levels.
     */
    uint32_t digest_levels;
  };

  /**
   * @brief scale-encodes blob instance to stream
   * @tparam Stream output stream type
   * @tparam size blob size
   * @param s output stream reference
   * @param blob value to encode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ChangesTrieConfig &config) {
    s << config.digest_interval << config.digest_levels;
    return s;
  }

  /**
   * @brief decodes blob instance from stream
   * @tparam Stream output stream type
   * @tparam size blob size
   * @param s input stream reference
   * @param blob value to encode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ChangesTrieConfig &config) {
    s >> config.digest_interval >> config.digest_levels;
    return s;
  }
}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG
