/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include "primitives/block_id.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using kagome::primitives::BlockNumber;
using kagome::primitives::Transaction;

namespace kagome::transaction_pool {

  TransactionPoolImpl::TransactionPoolImpl(
      std::unique_ptr<PoolModerator> moderator,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      Limits limits)
      : header_repo_{std::move(header_repo)},
        moderator_{std::move(moderator)},
        limits_{limits} {
    BOOST_ASSERT_MSG(header_repo_ != nullptr, "header repo is nullptr");
    BOOST_ASSERT_MSG(moderator_ != nullptr, "moderator is nullptr");
  }

  outcome::result<void> TransactionPoolImpl::submitOne(Transaction &&tx) {
    return submitOne(std::make_shared<Transaction>(std::move(tx)));
  }

  outcome::result<void> TransactionPoolImpl::submit(
      std::vector<Transaction> txs) {
    for (auto &tx : txs) {
      OUTCOME_TRY(submitOne(std::make_shared<Transaction>(std::move(tx))));
    }

    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::submitOne(
      const std::shared_ptr<Transaction> &tx) {
    if (auto [_, ok] = imported_txs_.emplace(tx->hash, tx); !ok) {
      return TransactionPoolError::TX_ALREADY_IMPORTED;
    }

    auto processResult = processTransaction(tx);
    if (processResult.has_error()
        && processResult.error() == TransactionPoolError::POOL_IS_FULL) {
      imported_txs_.erase(tx->hash);
    } else {
      logger_->debug("Extrinsic {} with hash {} was added to the pool",
                     tx->ext.data.toHex(),
                     tx->hash.toHex());
    }

    return processResult;
  }

  outcome::result<void> TransactionPoolImpl::processTransaction(
      const std::shared_ptr<Transaction> &tx) {
    OUTCOME_TRY(ensureSpace());
    if (checkForReady(tx)) {
      OUTCOME_TRY(processTransactionAsReady(tx));
    } else {
      OUTCOME_TRY(processTransactionAsWaiting(tx));
    }
    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::processTransactionAsReady(
      const std::shared_ptr<Transaction> &tx) {
    if (hasSpaceInReady()) {
      setReady(tx);
    } else {
      postponeTransaction(tx);
    }

    return outcome::success();
  }

  bool TransactionPoolImpl::hasSpaceInReady() const {
    return ready_txs_.size() < limits_.max_ready_num;
  }

  void TransactionPoolImpl::postponeTransaction(
      const std::shared_ptr<Transaction> &tx) {
    postponed_txs_.push_back(tx);
  }

  outcome::result<void> TransactionPoolImpl::processTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    OUTCOME_TRY(ensureSpace());

    addTransactionAsWaiting(tx);

    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::ensureSpace() const {
    if (imported_txs_.size() > limits_.capacity) {
      return TransactionPoolError::POOL_IS_FULL;
    }

    return outcome::success();
  }

  void TransactionPoolImpl::addTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      tx_waits_tag_.emplace(tag, tx);
    }
  }

  outcome::result<void> TransactionPoolImpl::removeOne(
      const Transaction::Hash &tx_hash) {
    auto tx_node = imported_txs_.extract(tx_hash);
    if (tx_node.empty()) {
      logger_->debug(
          "Extrinsic with hash {} was not found in the pool during remove",
          tx_hash);
      return TransactionPoolError::TX_NOT_FOUND;
    }
    auto &tx = tx_node.mapped();

    unsetReady(tx);
    delTransactionAsWaiting(tx);

    processPostponedTransactions();

    logger_->debug("Extrinsic {} with hash {} was removed from the pool",
                   tx->ext.data.toHex(),
                   tx->hash.toHex());
    return outcome::success();
  }

  outcome::result<void> TransactionPoolImpl::remove(
      const std::vector<Transaction::Hash> &tx_hashes) {
    for (auto &tx_hash : tx_hashes) {
      removeOne(tx_hash);
    }

    return outcome::success();
  }

  void TransactionPoolImpl::processPostponedTransactions() {
    // Move to local for avoid endless cycle at possible coming back tx
    auto postponed_txs = std::move(postponed_txs_);
    while (!postponed_txs.empty()) {
      auto tx = postponed_txs.front().lock();
      postponed_txs.pop_front();

      auto result = processTransaction(tx);
      if (result.has_error()
          && result.error() == TransactionPoolError::POOL_IS_FULL) {
        postponed_txs_.insert(postponed_txs_.end(),
                              std::make_move_iterator(postponed_txs.begin()),
                              std::make_move_iterator(postponed_txs.end()));
        return;
      }
    }
  }

  void TransactionPoolImpl::delTransactionAsWaiting(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      auto range = tx_waits_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second;) {
        if (i->second.lock() == tx) {
          tx_waits_tag_.erase(i);
          break;
        }
      }
    }
  }

  std::map<Transaction::Hash, std::shared_ptr<Transaction>>
  TransactionPoolImpl::getReadyTransactions() const {
    std::map<Transaction::Hash, std::shared_ptr<Transaction>> ready;
    std::for_each(ready_txs_.begin(), ready_txs_.end(), [&ready](auto it) {
      if (auto tx = it.second.lock()) {
        ready.emplace(it.first, std::move(tx));
      }
    });
    return ready;
  }

  outcome::result<std::vector<Transaction>> TransactionPoolImpl::removeStale(
      const primitives::BlockId &at) {
    OUTCOME_TRY(number, header_repo_->getNumberById(at));

    std::vector<Transaction::Hash> remove_to;

    for (auto &[txHash, tx] : imported_txs_) {
      if (moderator_->banIfStale(number, *tx)) {
        remove_to.emplace_back(txHash);
      }
    }

    for (auto &tx_hash : remove_to) {
      OUTCOME_TRY(removeOne(tx_hash));
    }

    moderator_->updateBan();

    return outcome::success();
  }

  bool TransactionPoolImpl::isInReady(
      const std::shared_ptr<const Transaction> &tx) const {
    auto i = ready_txs_.find(tx->hash);
    return i != ready_txs_.end() && !i->second.expired();
  }

  bool TransactionPoolImpl::checkForReady(
      const std::shared_ptr<const Transaction> &tx) const {
    return std::all_of(
        tx->requires.begin(), tx->requires.end(), [this](auto &&tag) {
          auto range = tx_provides_tag_.equal_range(tag);
          return range.first != range.second;
        });
  }

  void TransactionPoolImpl::setReady(const std::shared_ptr<Transaction> &tx) {
    if (auto [_, ok] = ready_txs_.emplace(tx->hash, tx); ok) {
      commitRequiredTags(tx);
      commitProvidedTags(tx);
    }
  }

  void TransactionPoolImpl::commitRequiredTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      auto range = tx_waits_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second;) {
        auto ci = i++;
        if (ci->second.lock() == tx) {
          auto node = tx_waits_tag_.extract(ci);
          tx_depends_on_tag_.emplace(std::move(node.key()),
                                     std::move(node.mapped()));
        }
      }
    }
  }

  void TransactionPoolImpl::commitProvidedTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->provides) {
      tx_provides_tag_.emplace(tag, tx);

      provideTag(tag);
    }
  }

  void TransactionPoolImpl::provideTag(const Transaction::Tag &tag) {
    auto range = tx_waits_tag_.equal_range(tag);
    for (auto it = range.first; it != range.second;) {
      auto tx = (it++)->second.lock();
      if (checkForReady(tx)) {
        if (hasSpaceInReady()) {
          setReady(tx);
        } else {
          postponeTransaction(tx);
        }
      }
    }
  }

  void TransactionPoolImpl::unsetReady(const std::shared_ptr<Transaction> &tx) {
    if (auto tx_node = ready_txs_.extract(tx->hash); !tx_node.empty()) {
      rollbackRequiredTags(tx);
      rollbackProvidedTags(tx);
    }
  }

  void TransactionPoolImpl::rollbackRequiredTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->requires) {
      tx_waits_tag_.emplace(tag, tx);
    }
  }

  void TransactionPoolImpl::rollbackProvidedTags(
      const std::shared_ptr<Transaction> &tx) {
    for (auto &tag : tx->provides) {
      auto range = tx_provides_tag_.equal_range(tag);
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second.lock() == tx) {
          tx_provides_tag_.erase(i);
          break;
        }
      }

      unprovideTag(tag);
    }
  }

  void TransactionPoolImpl::unprovideTag(const Transaction::Tag &tag) {
    if (tx_provides_tag_.find(tag) == tx_provides_tag_.end()) {
      for (auto it = tx_depends_on_tag_.find(tag);
           it != tx_depends_on_tag_.end();
           it = tx_depends_on_tag_.find(tag)) {
        if (auto tx = it->second.lock()) {
          unsetReady(tx);
        }
        tx_depends_on_tag_.erase(it);
      }
    }
  }

  TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const {
    return Status{ready_txs_.size(), imported_txs_.size() - ready_txs_.size()};
  }

}  // namespace kagome::transaction_pool
