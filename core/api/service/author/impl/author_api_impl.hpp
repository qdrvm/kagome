/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_IMPL_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_IMPL_HPP

/**
 * ExtrinsicSubmissionApi based on auth api implemented in substrate here
 * https://github.com/paritytech/substrate/blob/e8739300ae3f7f2e7b72f64668573275f2806ea5/core/rpc/src/author/mod.rs#L50-L49
 */

#include <boost/variant.hpp>
#include <outcome/outcome.hpp>

#include "api/service/author/error.hpp"
#include "api/service/author/author_api.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "common/visitor.hpp"
#include "crypto/hasher.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::runtime {
  class TaggedTransactionQueue;
}

namespace kagome::api {
  class AuthorApiImpl : public AuthorApi {
    template <class T>
    using sptr = std::shared_ptr<T>;

   public:
    /**
     * @constructor
     * @param api ttq instance shared ptr
     * @param pool transaction pool instance shared ptr
     * @param hasher hasher instance shared ptr
     * @param block_tree block tree instance shared ptr
     */
    AuthorApiImpl(std::shared_ptr<runtime::TaggedTransactionQueue> api,
                     std::shared_ptr<transaction_pool::TransactionPool> pool,
                     std::shared_ptr<crypto::Hasher> hasher,
                     std::shared_ptr<blockchain::BlockTree> block_tree,
                     std::shared_ptr<storage::trie::TrieDb> trie_db);

    ~AuthorApiImpl() override = default;

    outcome::result<common::Hash256> submitExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<std::vector<primitives::Extrinsic>> pendingExtrinsics()
        override;

    // TODO(yuraz): probably will be documented later (no task yet)
    outcome::result<std::vector<common::Hash256>> removeExtrinsic(
        const std::vector<primitives::ExtrinsicKey> &keys) override;

   private:
    sptr<runtime::TaggedTransactionQueue> api_;
    sptr<transaction_pool::TransactionPool> pool_;
    sptr<crypto::Hasher> hasher_;
    sptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::trie::TrieDb> trie_db_;
    common::Logger logger_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_IMPL_HPP
