/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/predicate.hpp>

#include "network/impl/state_sync_request_flow.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_api/core.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, StateSyncRequestFlow::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::EMPTY_RESPONSE:
      return "State sync empty response";
    case E::HASH_MISMATCH:
      return "State sync hash mismatch";
  }
  abort();
}

namespace kagome::network {
  StateSyncRequestFlow::StateSyncRequestFlow(
      const primitives::BlockInfo &block_info,
      const primitives::BlockHeader &block)
      : block_info_{block_info}, block_{block} {
    roots_[std::nullopt];
  }

  bool StateSyncRequestFlow::complete() const {
    return complete_roots_.size() == roots_.size();
  }

  StateRequest StateSyncRequestFlow::nextRequest() const {
    assert(not complete());
    return StateRequest{block_info_.hash, last_key_, true};
  }

  outcome::result<void> StateSyncRequestFlow::onResponse(
      const StateResponse &res) {
    assert(not complete());
    auto empty = true;
    for (auto &entry : res.entries) {
      if (not entry.entries.empty() or entry.complete) {
        empty = false;
      }
    }
    if (empty) {
      return Error::EMPTY_RESPONSE;
    }
    if (last_key_.size() == 2 && res.entries[0].entries.empty()) {
      last_key_.pop_back();
    } else {
      last_key_.resize(0);
    }
    for (auto &entry : res.entries) {
      if (not entry.complete) {
        if (not entry.entries.empty()) {
          last_key_.emplace_back(entry.entries.back().key);
        }
      }
      auto is_top = not entry.state_root;
      if (complete_roots_.count(entry.state_root)) {
        continue;
      }
      auto &root = roots_[entry.state_root];
      for (auto &[key, value] : entry.entries) {
        if (is_top && boost::starts_with(key, storage::kChildStoragePrefix)) {
          OUTCOME_TRY(hash, RootHash::fromSpan(value));
          roots_[hash].children.emplace_back(key);
        } else {
          root.trie.put(key, common::BufferView{value}).value();
        }
      }
      if (entry.complete) {
        complete_roots_.emplace(entry.state_root);
      }
    }
    return outcome::success();
  }

  outcome::result<void> StateSyncRequestFlow::commit(
      const runtime::ModuleFactory &module_factory,
      runtime::Core &core_api,
      storage::trie::TrieSerializer &trie_serializer) {
    assert(complete());
    auto &top = roots_[std::nullopt];
    OUTCOME_TRY(code, top.trie.get(storage::kRuntimeCodeKey));
    OUTCOME_TRY(env,
                runtime::RuntimeEnvironment::fromCode(module_factory, code));
    OUTCOME_TRY(runtime_version, core_api.version(env));
    auto version = storage::trie::StateVersion{runtime_version.state_version};
    for (auto &[expected, root] : roots_) {
      if (not expected) {
        continue;
      }
      OUTCOME_TRY(actual, trie_serializer.storeTrie(root.trie, version));
      if (actual != expected) {
        return Error::HASH_MISMATCH;
      }
      for (auto &child : root.children) {
        top.trie.put(child, common::BufferView{actual}).value();
      }
    }
    OUTCOME_TRY(actual, trie_serializer.storeTrie(top.trie, version));
    if (actual != block_.state_root) {
      return Error::HASH_MISMATCH;
    }
    return outcome::success();
  }
}  // namespace kagome::network
