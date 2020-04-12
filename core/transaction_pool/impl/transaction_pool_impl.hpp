/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_POOL_IMPL_HPP
#define KAGOME_TRANSACTION_POOL_IMPL_HPP

#include <outcome/outcome.hpp>
#include "blockchain/block_header_repository.hpp"
#include "common/logger.hpp"
#include "transaction_pool/pool_moderator.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool
{

class TransactionPoolImpl : public TransactionPool
{
	static constexpr auto kDefaultLoggerTag = "Transaction Pool";

public:
	TransactionPoolImpl(
		std::unique_ptr<PoolModerator> moderator,
		std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
		Limits limits
	);

	TransactionPoolImpl(TransactionPoolImpl&&) = default;
	TransactionPoolImpl(const TransactionPoolImpl&) = delete;

	~TransactionPoolImpl() override = default;

	TransactionPoolImpl& operator=(TransactionPoolImpl&&) = delete;
	TransactionPoolImpl& operator=(const TransactionPoolImpl&) = delete;

	outcome::result<void> submitOne(Transaction&& tx) override;
	outcome::result<void> submit(std::vector<Transaction> txs) override;

	outcome::result<void> removeOne(const Transaction::Hash& txHash) override;
	outcome::result<void> remove(const std::vector<Transaction::Hash>& txHashes) override;

	std::vector<Transaction> getReadyTransactions() override;

	outcome::result<std::vector<Transaction>> removeStale(const primitives::BlockId& at) override;

	Status getStatus() const override;

private:
	outcome::result<void> submitOne(const std::shared_ptr<Transaction>& tx);

	outcome::result<void> processTx(const std::shared_ptr<Transaction>& tx);

	outcome::result<void> processTxAsReady(const std::shared_ptr<Transaction>& tx);

	outcome::result<void> processTxAsWaiting(const std::shared_ptr<Transaction>& tx);

	bool hasSpaceInReady() const;

	outcome::result<void> ensureSpaceInWaiting() const;

	void addTxAsWaiting(const std::shared_ptr<Transaction>& tx);

	void delTxAsWaiting(const std::shared_ptr<Transaction>& tx);

	void postponeTx(const std::shared_ptr<Transaction>& tx);

	void processPostponedTxs();

	void provideTag(const Transaction::Tag& tag);

	void unprovideTag(const Transaction::Tag& tag);

	void commitRequires(const std::shared_ptr<Transaction>& tx);

	void commitProvides(const std::shared_ptr<Transaction>& tx);

	void rollbackRequires(const std::shared_ptr<Transaction>& tx);

	void rollbackProvides(const std::shared_ptr<Transaction>& tx);

	bool checkForReady(const std::shared_ptr<const Transaction>& tx) const;

	void setReady(const std::shared_ptr<Transaction>& tx);

	void unsetReady(const std::shared_ptr<Transaction>& tx);

	bool isReady(const std::shared_ptr<const Transaction>& tx) const;

	std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;

	common::Logger logger_ = common::createLogger(kDefaultLoggerTag);

	// bans stale and invalid transactions for some amount of time
	std::unique_ptr<PoolModerator> moderator_;

	std::unordered_map<Transaction::Hash, std::shared_ptr<Transaction>> _importedTxs{};

	std::unordered_map<Transaction::Hash, std::weak_ptr<Transaction>> _readyTxs{};
	std::list<std::weak_ptr<Transaction>> _postponedTxs{};

	std::multimap<Transaction::Tag, std::weak_ptr<Transaction>> _txProvideTag{};

	std::multimap<Transaction::Tag, std::weak_ptr<Transaction>> _txDependentTag{};
	std::multimap<Transaction::Tag, std::weak_ptr<Transaction>> _txWaitsTag{};


	Limits limits_;
};

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TRANSACTION_POOL_IMPL_HPP
