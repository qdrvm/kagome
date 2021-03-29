/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/impl/author_api_impl.hpp"

#include <jsonrpc-lean/fault.h>
#include <boost/assert.hpp>
#include <boost/system/error_code.hpp>

#include "common/visitor.hpp"
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "subscription/subscriber.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {
  AuthorApiImpl::AuthorApiImpl(
      sptr<runtime::TaggedTransactionQueue> api,
      sptr<transaction_pool::TransactionPool> pool,
      sptr<crypto::Hasher> hasher,
      std::shared_ptr<network::ExtrinsicGossiper> gossiper)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)},
        gossiper_{std::move(gossiper)},
        last_id_{0},
        logger_{log::createLogger("AuthorApi", "author_api")} {
    BOOST_ASSERT_MSG(api_ != nullptr, "author api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(gossiper_ != nullptr, "gossiper is nullptr");
    BOOST_ASSERT_MSG(logger_ != nullptr, "logger is nullptr");
  }

  void AuthorApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<common::Hash256> AuthorApiImpl::submitExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    OUTCOME_TRY(tx, constructTransaction(extrinsic, boost::none));

    if (tx.should_propagate) {
      gossiper_->propagateTransactions(gsl::make_span(std::vector{tx}));
    }
    auto hash = tx.hash;
    // send to pool
    OUTCOME_TRY(pool_->submitOne(std::move(tx)));
    return hash;
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  AuthorApiImpl::pendingExtrinsics() {
    auto &pending_txs = pool_->getPendingTransactions();

    std::vector<primitives::Extrinsic> result;
    result.reserve(pending_txs.size());

    std::transform(pending_txs.begin(),
                   pending_txs.end(),
                   std::back_inserter(result),
                   [](auto &it) { return it.second->ext; });

    return result;
  }

  outcome::result<std::vector<common::Hash256>> AuthorApiImpl::removeExtrinsic(
      const std::vector<primitives::ExtrinsicKey> &keys) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }

  outcome::result<AuthorApi::SubscriptionId>
  AuthorApiImpl::submitAndWatchExtrinsic(Extrinsic extrinsic) {
    OUTCOME_TRY(tx, constructTransaction(extrinsic, last_id_++));

    SubscriptionId sub_id{};
    if (auto service = api_service_.lock()) {
      OUTCOME_TRY(sub_id_, service->subscribeForExtrinsicLifecycle(tx));
      sub_id = sub_id_;
    } else {
      throw jsonrpc::InternalErrorFault(
          "Internal error. Api service not initialized.");
    }
    if (tx.should_propagate) {
      gossiper_->propagateTransactions(gsl::make_span(std::vector{tx}));
    }

    // send to pool
    OUTCOME_TRY(pool_->submitOne(std::move(tx)));

    return sub_id;
  }

  outcome::result<bool> AuthorApiImpl::unwatchExtrinsic(SubscriptionId sub_id) {
    if (auto service = api_service_.lock()) {
      return service->unsubscribeFromExtrinsicLifecycle(sub_id);
    }
    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<primitives::Transaction> AuthorApiImpl::constructTransaction(
      primitives::Extrinsic extrinsic,
      boost::optional<primitives::Transaction::ObservedId> id) const {
    OUTCOME_TRY(res,
                api_->validate_transaction(
                    primitives::TransactionSource::External, extrinsic));

    return visit_in_place(
        res,
        [&](const primitives::TransactionValidityError &e) {
          return visit_in_place(
              e,
              // return either invalid or unknown validity error
              [](const auto &validity_error)
                  -> outcome::result<primitives::Transaction> {
                return validity_error;
              });
        },
        [&](const primitives::ValidTransaction &v)
            -> outcome::result<primitives::Transaction> {
          common::Hash256 hash = hasher_->blake2b_256(extrinsic.data);
          size_t length = extrinsic.data.size();

          return primitives::Transaction{id,
                                         extrinsic,
                                         length,
                                         hash,
                                         v.priority,
                                         v.longevity,
                                         v.requires,
                                         v.provides,
                                         v.propagate};
        });
  }

}  // namespace kagome::api
