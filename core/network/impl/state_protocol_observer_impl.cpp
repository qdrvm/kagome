/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/state_protocol_observer_impl.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind/storage.hpp>
#include <libp2p/outcome/outcome.hpp>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "network/types/state_response.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie/types.hpp"

constexpr unsigned MAX_RESPONSE_BYTES = 2 * 1024 * 1024;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network,
                            StateProtocolObserverImpl::Error,
                            e) {
  using E = kagome::network::StateProtocolObserverImpl::Error;
  switch (e) {
    case E::INVALID_CHILD_ROOTHASH:
      return "Expected child root hash prefix.";
    case E::NOTFOUND_CHILD_ROOTHASH:
      return "Child storage root hash not found.";
  }
  return "unknown error";
}

namespace kagome::network {

  StateProtocolObserverImpl::StateProtocolObserverImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
      std::shared_ptr<storage::trie::TrieStorage> storage)
      : blocks_headers_(std::move(blocks_headers)),
        storage_{std::move(storage)},
        log_(log::createLogger("StateProtocolObserver", "network")) {
    BOOST_ASSERT(blocks_headers_);
    BOOST_ASSERT(storage_);
  }

  outcome::result<std::pair<KeyValueStateEntry, size_t>>
  StateProtocolObserverImpl::getEntry(const storage::trie::RootHash &hash,
                                      const common::Buffer &key,
                                      size_t limit) const {
    OUTCOME_TRY(batch, storage_->getEphemeralBatchAt(hash));

    auto cursor = batch->trieCursor();

    KeyValueStateEntry entry;
    entry.state_root = hash;

    auto res = key.empty() ? cursor->next() : cursor->seekUpperBound(key);
    size_t size = 0;
    if (res.has_value()) {
      while (cursor->key().has_value() && size < limit) {
        if (auto value_res = batch->tryGet(cursor->key().value());
            value_res.has_value()) {
          const auto &value_opt = value_res.value();
          if (value_opt.has_value()) {
            entry.entries.emplace_back(
                StateEntry{cursor->key().value(), value_opt.value()});
            size += entry.entries.back().key.size()
                    + entry.entries.back().value.size();
          }
        }
        res = cursor->next();
      }
      entry.complete = not cursor->key().has_value();
    } else {
      return res.as_failure();
    }

    return {entry, size};
  }

  outcome::result<network::StateResponse>
  StateProtocolObserverImpl::onStateRequest(const StateRequest &request) const {
    OUTCOME_TRY(header, blocks_headers_->getBlockHeader(request.hash));
    OUTCOME_TRY(batch, storage_->getEphemeralBatchAt(header.state_root));

    auto cursor = batch->trieCursor();
    // if key is not empty continue iteration from place where left
    auto res = (request.start.empty() || request.start[0].empty()
                    ? cursor->next()
                    : cursor->seekUpperBound(request.start[0]));
    unsigned size = 0;
    KeyValueStateEntry entry;
    // main state storage hash is marked with zeros in response
    entry.state_root =
        storage::trie::RootHash::fromSpan(std::vector<uint8_t>(32, 0)).value();

    StateResponse response;
    response.entries.emplace_back(std::move(entry));

    // First key would contain main state storage key (child state storage hash)
    // Second key is child state storage key
    if (request.start.size() == 2) {
      const auto &parent_key = request.start[0];
      const auto &child_prefix = storage::kChildStorageDefaultPrefix;
      if (!boost::starts_with(parent_key, child_prefix)) {
        return Error::INVALID_CHILD_ROOTHASH;
      }
      if (auto value_res = batch->tryGet(parent_key);
          value_res.has_value() && value_res.value().has_value()) {
        OUTCOME_TRY(
            hash,
            storage::trie::RootHash::fromSpan(value_res.value().value().get()));
        OUTCOME_TRY(
            entry_res,
            this->getEntry(hash, request.start[1], MAX_RESPONSE_BYTES - size));
        response.entries.emplace_back(std::move(entry_res.first));
        size += entry_res.second;
      } else {
        return Error::NOTFOUND_CHILD_ROOTHASH;
      }
    }

    if (!res.has_value()) {
      return res.as_failure();
    }

    const auto &child_prefix = storage::kChildStorageDefaultPrefix;
    while (cursor->key().has_value() && size < MAX_RESPONSE_BYTES) {
      if (auto value_res = batch->tryGet(cursor->key().value());
          value_res.has_value()) {
        const auto &value = value_res.value();
        auto &entry = response.entries.front();
        entry.entries.emplace_back(
            StateEntry{cursor->key().value(), value.value()});
        size +=
            entry.entries.back().key.size() + entry.entries.back().value.size();
        // if key is child state storage hash iterate child storage keys
        if (boost::starts_with(cursor->key().value(), child_prefix)) {
          OUTCOME_TRY(hash,
                      storage::trie::RootHash::fromSpan(
                          value_res.value().value().get()));
          OUTCOME_TRY(entry_res,
                      this->getEntry(
                          hash, common::Buffer(), MAX_RESPONSE_BYTES - size));
          response.entries.emplace_back(std::move(entry_res.first));
          size += entry_res.second;
          // not complete means response bytes limit exceeded
          // finish response formation
          if (not entry_res.first.complete) {
            break;
          }
        }
      }
      res = cursor->next();
    }
    response.entries.front().complete = not cursor->key().has_value();

    return response;
  }
}  // namespace kagome::network
