/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG
#define KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG

#include <cstdint>

namespace kagome::storage::changes_trie {
  /// https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/core/src/changes_trie.rs#L28
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

    bool operator==(const ChangesTrieConfig &rhs) const {
      return digest_interval == rhs.digest_interval
             and digest_levels == rhs.digest_levels;
    }

    bool operator!=(const ChangesTrieConfig &rhs) const {
      return !operator==(rhs);
    }
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
    return s << config.digest_interval << config.digest_levels;
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
    return s >> config.digest_interval >> config.digest_levels;
  }
}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_STORAGE_CHANGES_TRIE_CHANGES_TRIE_CONFIG
