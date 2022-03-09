/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/changes_trie/impl/changes_trie.hpp"

#include "scale/scale.hpp"

namespace kagome::storage::changes_trie {

  outcome::result<std::unique_ptr<ChangesTrie>> ChangesTrie::buildFromChanges(
      const primitives::BlockNumber &parent_number,
      const std::shared_ptr<storage::trie::PolkadotTrieFactory> &trie_factory,
      std::shared_ptr<storage::trie::Codec> codec,
      const ExtrinsicsChanges &extinsics_changes,
      const ChangesTrieConfig &config) {
    BOOST_ASSERT(trie_factory != nullptr);
    auto changes_storage = trie_factory->createEmpty();

    for (auto &change : extinsics_changes) {
      auto &key = change.first;
      auto &changers = change.second;
      auto current_number = parent_number + 1;
      KeyIndexVariant keyIndex{ExtrinsicsChangesKey{{current_number, key}}};
      OUTCOME_TRY(key_enc, scale::encode(keyIndex));
      OUTCOME_TRY(value, scale::encode(changers));
      common::Buffer value_buf{std::move(value)};
      OUTCOME_TRY(changes_storage->put(common::Buffer{std::move(key_enc)},
                                       std::move(value_buf)));
    }

    return std::unique_ptr<ChangesTrie>{
        new ChangesTrie{std::move(changes_storage), std::move(codec)}};
  }

  ChangesTrie::ChangesTrie(std::unique_ptr<storage::trie::PolkadotTrie> trie,
                           std::shared_ptr<storage::trie::Codec> codec)
      : changes_trie_{std::move(trie)},
        codec_{std::move(codec)},
        logger_(log::createLogger("ChangesTrie", "changes_trie")) {
    BOOST_ASSERT(changes_trie_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
  }

  common::Hash256 ChangesTrie::getHash() const {
    if (changes_trie_ == nullptr) {
      return {};
    }

    auto root = changes_trie_->getRoot();
    if (root == nullptr) {
      logger_->warn("Get root of empty changes trie");
      return codec_->hash256(common::Buffer{0});
    }
    auto enc_res = codec_->encodeNode(*root);
    if (enc_res.has_error()) {
      logger_->error("Encoding Changes trie failed"
                     + enc_res.error().message());
      return {};
    }
    auto hash_bytes = codec_->hash256(enc_res.value());
    return hash_bytes;
  }

}  // namespace kagome::storage::changes_trie
