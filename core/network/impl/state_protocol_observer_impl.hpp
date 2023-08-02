/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_PROTOCOL_OBSERVER_IMPL
#define KAGOME_STATE_PROTOCOL_OBSERVER_IMPL

#include "blockchain/block_header_repository.hpp"
#include "network/state_protocol_observer.hpp"

#include "log/logger.hpp"
#include "network/types/state_response.hpp"
#include "storage/trie/types.hpp"

namespace kagome {
  namespace blockchain {
    class BlockHeaderRepository;
  }
  namespace storage::trie {
    class TrieStorage;
  }
}  // namespace kagome

namespace kagome::network {

  class StateProtocolObserverImpl
      : public StateProtocolObserver,
        public std::enable_shared_from_this<StateProtocolObserverImpl> {
   public:
    enum class Error {
      INVALID_CHILD_ROOTHASH = 1,
      NOTFOUND_CHILD_ROOTHASH,
      VALUE_NOT_FOUND,
    };

    StateProtocolObserverImpl(
        std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers,
        std::shared_ptr<storage::trie::TrieStorage> storage);

    ~StateProtocolObserverImpl() override = default;

    outcome::result<StateResponse> onStateRequest(
        const StateRequest &request) const override;

   private:
    outcome::result<std::pair<KeyValueStateEntry, size_t>> getEntry(
        const storage::trie::RootHash &hash,
        const common::Buffer &key,
        size_t limit) const;

    outcome::result<common::Buffer> prove(
        const common::Hash256 &root,
        const std::vector<common::Buffer> &keys) const;

    std::shared_ptr<blockchain::BlockHeaderRepository> blocks_headers_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    log::Logger log_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, StateProtocolObserverImpl::Error);

#endif  // KAGOME_STATE_PROTOCOL_OBSERVER_IMPL
