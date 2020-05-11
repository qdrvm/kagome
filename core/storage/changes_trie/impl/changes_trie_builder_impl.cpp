/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/changes_trie/impl/changes_trie_builder_impl.hpp"

#include "scale/scale.hpp"
#include "storage/trie/impl/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::changes_trie,
                            ChangesTrieBuilderImpl::Error,
                            e) {
  using E = kagome::storage::changes_trie::ChangesTrieBuilderImpl::Error;

  switch (e) {
    case E::TRIE_NOT_INITIALIZED:
      return "No changes trie has been initialized; Supposedly startNewTrie "
             "was not called;";
  }
  return "Unknown error";
}

namespace kagome::storage::changes_trie {

  const common::Buffer ChangesTrieBuilderImpl::CHANGES_CONFIG_KEY{[]() {
    constexpr auto s = ":changes_trie";
    std::vector<uint8_t> v(strlen(s));
    std::copy(s, s + strlen(s), v.begin());
    return v;
  }()};

  ChangesTrieBuilderImpl::ChangesTrieBuilderImpl(
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::unique_ptr<storage::trie::PolkadotTrie> changes_storage,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::trie::Codec> codec)
      : storage_{std::move(storage)},
        block_header_repo_{std::move(block_header_repo)},
        changes_storage_{std::move(changes_storage)},
        codec_{std::move(codec)},
        logger_ {common::createLogger("Changes Trie Builder")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(changes_storage_ != nullptr);
    BOOST_ASSERT(block_header_repo_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
  }

  outcome::result<ChangesTrieBuilderImpl::OptChangesTrieConfig>
  ChangesTrieBuilderImpl::getConfig() const {
    /// TODO(Harrm): Update config from recent changes(might be not commited)
    OUTCOME_TRY(batch, storage_->getEphemeralBatch());
    auto config_bytes_res = batch->get(CHANGES_CONFIG_KEY);
    if (config_bytes_res.has_error()) {
      if (config_bytes_res.error() == storage::trie::TrieError::NO_VALUE) {
        return boost::none;
      } else {
        return config_bytes_res.error();
      }
    }
    OUTCOME_TRY(config,
                scale::decode<ChangesTrieConfig>(config_bytes_res.value()));
    return config;
  }

  outcome::result<std::reference_wrapper<ChangesTrieBuilder>>
  ChangesTrieBuilderImpl::startNewTrie(const common::Hash256 &parent) {
    OUTCOME_TRY(changes_storage_->clearPrefix({}));
    parent_hash_ = parent;
    OUTCOME_TRY(parent_number,
                block_header_repo_->getNumberByHash(parent_hash_));
    parent_number_ = parent_number;
    return *this;
  }

  outcome::result<void> ChangesTrieBuilderImpl::insertExtrinsicsChange(
      const common::Buffer &key,
      const std::vector<primitives::ExtrinsicIndex> &changers) {
    if (changes_storage_ == nullptr) {
      return Error::TRIE_NOT_INITIALIZED;
    }
    auto current_number = parent_number_ + 1;
    KeyIndexVariant keyIndex{ExtrinsicsChangesKey{{current_number, key}}};
    OUTCOME_TRY(key_enc, scale::encode(keyIndex));
    OUTCOME_TRY(value, scale::encode(changers));
    OUTCOME_TRY(changes_storage_->put(common::Buffer{std::move(key_enc)},
                                      common::Buffer{std::move(value)}));
    return outcome::success();
  }

  common::Hash256 ChangesTrieBuilderImpl::finishAndGetHash() {
    if (changes_storage_ == nullptr) {
      return {};
    }
    auto root = changes_storage_->getRoot();
    if (root == nullptr) {
      auto hash_bytes = codec_->hash256({0});
    }
    auto enc_res = codec_->encodeNode(*root);
    if (enc_res.error()) {
      logger_->error("Encoding Changes trie failed" + enc_res.error().message());
      return {};
    }
    auto hash_bytes = codec_->hash256(enc_res.value());
    return hash_bytes;
  }

}  // namespace kagome::storage::changes_trie
