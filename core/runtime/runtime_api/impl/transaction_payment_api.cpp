/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/transaction_payment_api.hpp"

#include "crypto/hasher.hpp"
#include "runtime/executor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime,
                            TransactionPaymentApiImpl::Error,
                            err) {
  using E = kagome::runtime::TransactionPaymentApiImpl::Error;
  switch (err) {
    case E::TRANSACTION_PAYMENT_API_NOT_FOUND:
      return "Transaction payment runtime API is not found in the runtime";
    case TransactionPaymentApiImpl::Error::API_BELOW_VERSION_2_NOT_SUPPORTED:
      return "API below version 2 is not supported";
  }
  return "Unknown TransactionPaymentApiImpl error";
}

namespace kagome::runtime {

  TransactionPaymentApiImpl::TransactionPaymentApiImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<Core> core_api,
      std::shared_ptr<crypto::Hasher> hasher)
      : executor_{std::move(executor)}, core_api_{core_api}, hasher_{hasher} {
    BOOST_ASSERT(executor_);
    BOOST_ASSERT(core_api_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<primitives::RuntimeDispatchInfo<primitives::Weight>>
  TransactionPaymentApiImpl::query_info(const primitives::BlockHash &block,
                                        const primitives::Extrinsic &ext,
                                        uint32_t len) {
    OUTCOME_TRY(runtime_version, core_api_->version(block));

    static const common::Hash64 transaction_payment_api_hash =
        hasher_->blake2b_64(
            common::Buffer::fromString("TransactionPaymentApi"));
    const auto &c_transaction_payment_api_hash =
        transaction_payment_api_hash;  // to create memory storage to push in
                                       // lambda
    auto res = std::find_if(runtime_version.apis.begin(),
                            runtime_version.apis.end(),
                            [&](auto &api_version) {
                              return api_version.first
                                  == c_transaction_payment_api_hash;
                            });
    if (res == runtime_version.apis.end()) {
      return Error::TRANSACTION_PAYMENT_API_NOT_FOUND;
    }
    auto api_version = res->second;
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    if (api_version < 2) {
      return Error::API_BELOW_VERSION_2_NOT_SUPPORTED;
    }
    OUTCOME_TRY(
        result,
        executor_->call<primitives::RuntimeDispatchInfo<primitives::Weight>>(
            ctx, "TransactionPaymentApi_query_info", ext.data, len));
   
    return result;
  }

}  // namespace kagome::runtime
