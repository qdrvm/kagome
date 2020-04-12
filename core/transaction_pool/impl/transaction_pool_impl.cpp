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
	Limits limits
)
: header_repo_{std::move(header_repo)}
, moderator_{std::move(moderator)}
, limits_{limits}
{
	BOOST_ASSERT_MSG(header_repo_ != nullptr, "header repo is nullptr");
	BOOST_ASSERT_MSG(moderator_ != nullptr, "moderator is nullptr");
}

outcome::result<void> TransactionPoolImpl::submitOne(Transaction&& tx)
{
	return submitOne(std::make_shared<Transaction>(std::forward<Transaction>(tx)));
}

outcome::result<void> TransactionPoolImpl::submit(std::vector<Transaction> txs)
{
	for (auto &tx : txs)
	{
		OUTCOME_TRY(submitOne(std::make_shared<Transaction>(std::move(tx))));
	}

	return outcome::success();
}

outcome::result<void> TransactionPoolImpl::submitOne(const std::shared_ptr<Transaction>& tx)
{
	if (auto [_, ok] = _importedTxs.emplace(tx->hash, tx); !ok)
	{
		return TransactionPoolError::ALREADY_IMPORTED;
	}

	auto processResult = processTx(tx);
	if (processResult.has_error() && processResult.error() == TransactionPoolError::POOL_OVERFLOW)
	{
		_importedTxs.erase(tx->hash);
	}

	return processResult;
}

outcome::result<void> TransactionPoolImpl::processTx(const std::shared_ptr<Transaction>& tx)
{
	if (checkForReady(tx))
	{
		return processTxAsReady(tx);
	}
	else
	{
		return processTxAsWaiting(tx);
	}
}

outcome::result<void> TransactionPoolImpl::processTxAsReady(const std::shared_ptr<Transaction>& tx)
{
	if (hasSpaceInReady())
	{
		setReady(tx);
	}
	else
	{
		OUTCOME_TRY(ensureSpaceInWaiting());

		postponeTx(tx);
	}

	return outcome::success();
}

bool TransactionPoolImpl::hasSpaceInReady() const
{
	return _readyTxs.size() < limits_.max_ready_num;
}

void TransactionPoolImpl::postponeTx(const std::shared_ptr<Transaction>& tx)
{
	_postponedTxs.push_back(tx);
}

outcome::result<void> TransactionPoolImpl::processTxAsWaiting(const std::shared_ptr<Transaction>& tx)
{
	OUTCOME_TRY(ensureSpaceInWaiting());

	addTxAsWaiting(tx);

	return outcome::success();
}

outcome::result<void> TransactionPoolImpl::ensureSpaceInWaiting() const
{
	if (_importedTxs.size() - _readyTxs.size() > limits_.max_waiting_num)
	{
		return TransactionPoolError::POOL_OVERFLOW;
	}

	return outcome::success();
}

void TransactionPoolImpl::addTxAsWaiting(const std::shared_ptr<Transaction>& tx)
{
	for (auto& tag : tx->requires)
	{
		_txWaitsTag.emplace(tag, tx);
	}
}

outcome::result<void> TransactionPoolImpl::removeOne(const Transaction::Hash& txHash)
{
	auto tx_node = _importedTxs.extract(txHash);
	if (tx_node.empty())
	{
		return TransactionPoolError::NOT_FOUND;
	}
	auto& tx = tx_node.mapped();

	unsetReady(tx);
	delTxAsWaiting(tx);

	processPostponedTxs();

	return outcome::success();
}

outcome::result<void> TransactionPoolImpl::remove(const std::vector<Transaction::Hash>& txHashes)
{
	for (auto &txHash : txHashes)
	{
		OUTCOME_TRY(removeOne(txHash));
	}

	return outcome::success();
}

void TransactionPoolImpl::processPostponedTxs()
{
	auto postponedTxs = std::move(_postponedTxs);
	while (!postponedTxs.empty())
	{
		auto tx = postponedTxs.front().lock();
		postponedTxs.pop_front();

		auto result = processTx(tx);
		if (result.has_error() && result.error() == TransactionPoolError::POOL_OVERFLOW)
		{
			_postponedTxs.insert(
				_postponedTxs.end(),
				std::make_move_iterator(postponedTxs.begin()),
				std::make_move_iterator(postponedTxs.end())
			);
			return;
		}
	}
}

void TransactionPoolImpl::delTxAsWaiting(const std::shared_ptr<Transaction>& tx)
{
	for (auto& tag : tx->requires)
	{
		auto range = _txWaitsTag.equal_range(tag);
		for (auto i = range.first; i != range.second;)
		{
			if (i->second.lock() == tx)
			{
				_txWaitsTag.erase(i);
				break;
			}
		}
	}
}

std::vector<Transaction> TransactionPoolImpl::getReadyTransactions()
{
	std::vector<Transaction> ready(_readyTxs.size());
	std::transform(
		_readyTxs.begin(), _readyTxs.end(), ready.begin(),
		[](auto& rtx) {
			return *rtx.second.lock();
		}
	);
	return ready;
}

outcome::result<std::vector<Transaction>> TransactionPoolImpl::removeStale(const primitives::BlockId& at)
{
	OUTCOME_TRY(number, header_repo_->getNumberById(at));

	std::vector<Transaction::Hash> remove_to;

	for (auto&[txHash, tx] : _importedTxs)
	{
		if (moderator_->banIfStale(number, *tx))
		{
			remove_to.emplace_back(txHash);
		}
	}

	for (auto txHash : remove_to)
	{
		OUTCOME_TRY(removeOne(txHash));
	}

	moderator_->updateBan();

	return outcome::success();
}

bool TransactionPoolImpl::isReady(const std::shared_ptr<const Transaction> &tx) const
{
	auto i = _readyTxs.find(tx->hash);
	return i != _readyTxs.end() && !i->second.expired();
}

bool TransactionPoolImpl::checkForReady(const std::shared_ptr<const Transaction> &tx) const
{
	return std::all_of(
		tx->requires.begin(),
		tx->requires.end(),
		[this](auto&& tag) {
			auto range = _txProvideTag.equal_range(tag);
			return range.first != range.second;
		}
	);
}

void TransactionPoolImpl::setReady(const std::shared_ptr<Transaction> &tx)
{
	if (auto [_, ok] = _readyTxs.emplace(tx->hash, tx); ok)
	{
		commitRequires(tx);
		commitProvides(tx);
	}
}

void TransactionPoolImpl::commitRequires(const std::shared_ptr<Transaction> &tx)
{
	for (auto &tag : tx->requires)
	{
		auto range = _txWaitsTag.equal_range(tag);
		for (auto i = range.first; i != range.second;)
		{
			auto ci = i++;
			if (ci->second.lock() == tx)
			{
				auto node = _txWaitsTag.extract(ci);
				_txDependentTag.emplace(std::move(node.key()), std::move(node.mapped()));
			}
		}
	}
}

void TransactionPoolImpl::commitProvides(const std::shared_ptr<Transaction> &tx)
{
	for (auto& tag : tx->provides)
	{
		_txProvideTag.emplace(tag, tx);

		provideTag(tag);
	}
}

void TransactionPoolImpl::provideTag(const Transaction::Tag& tag)
{
	auto range = _txWaitsTag.equal_range(tag);
	for (auto it = range.first; it != range.second; )
	{
		auto tx = (it++)->second.lock();
		if (checkForReady(tx))
		{
			if (hasSpaceInReady())
			{
				setReady(tx);
			}
			else
			{
				postponeTx(tx);
			}
		}
	}
}

void TransactionPoolImpl::unsetReady(const std::shared_ptr<Transaction> &tx)
{
	if (auto tx_node = _readyTxs.extract(tx->hash); !tx_node.empty())
	{
		rollbackRequires(tx);
		rollbackProvides(tx);
	}
}

void TransactionPoolImpl::rollbackRequires(const std::shared_ptr<Transaction> &tx)
{
	for (auto &tag : tx->requires)
	{
		_txWaitsTag.emplace(tag, tx);
	}
}

void TransactionPoolImpl::rollbackProvides(const std::shared_ptr<Transaction> &tx)
{
	for (auto &tag : tx->provides)
	{
		auto range = _txProvideTag.equal_range(tag);
		for (auto i = range.first; i != range.second;)
		{
			if (i->second.lock() == tx)
			{
				_txProvideTag.erase(i);
				break;
			}
		}

		unprovideTag(tag);
	}
}

void TransactionPoolImpl::unprovideTag(const Transaction::Tag& tag)
{
	if (_txProvideTag.find(tag) == _txProvideTag.end())
	{
		for (auto it = _txDependentTag.find(tag); it !=  _txDependentTag.end(); it = _txDependentTag.find(tag))
		{
			if (auto tx = it->second.lock())
			{
				unsetReady(tx);
			}
			_txDependentTag.erase(it);
		}
	}
}

TransactionPoolImpl::Status TransactionPoolImpl::getStatus() const
{
	return Status{
		_readyTxs.size(),
		_importedTxs.size() - _readyTxs.size()
	};
}

}  // namespace kagome::transaction_pool
