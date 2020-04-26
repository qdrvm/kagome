/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/changes_trie_builder_impl.hpp"

#include "scale/scale.hpp"
#include "storage/trie/impl/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain,
                            ChangesTrieBuilderImpl::Error,
                            e) {
  using E = kagome::blockchain::ChangesTrieBuilderImpl::Error;

  switch (e) {
    case E::TRIE_NOT_INITIALIZED:
      return "No changes trie has been initialized; Supposedly startNewTrie "
             "was not called;";
  }
  return "Unknown error";
}

namespace kagome::blockchain {

  const common::Buffer ChangesTrieBuilderImpl::CHANGES_CONFIG_KEY{[]() {
    constexpr auto s = ":changes_trie";
    std::vector<uint8_t> v(strlen(s));
    std::copy(s, s + strlen(s), v.begin());
    return v;
  }()};

  ChangesTrieBuilderImpl::ChangesTrieBuilderImpl(
      std::shared_ptr<storage::trie::TrieDb> storage,
      std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo)
      : storage_{std::move(storage)},
        changes_storage_factory_{std::move(changes_storage_factory)},
        block_header_repo_{std::move(block_header_repo)} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(changes_storage_factory_ != nullptr);
    BOOST_ASSERT(block_header_repo_ != nullptr);
    changes_storage_ = nullptr;
  }

  outcome::result<ChangesTrieBuilderImpl::OptChangesTrieConfig>
  ChangesTrieBuilderImpl::getConfig() const {
    auto config_bytes_res = storage_->get(CHANGES_CONFIG_KEY);
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
    changes_storage_ = changes_storage_factory_->makeTrieDb();
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
    auto hash_bytes = changes_storage_->getRootHash();
    changes_storage_.reset();
    common::Hash256 hash;
    std::copy_n(hash_bytes.begin(), common::Hash256::size(), hash.begin());
    return hash;
  }

}  // namespace kagome::blockchain
