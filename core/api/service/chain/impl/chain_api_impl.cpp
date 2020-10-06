/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/chain/impl/chain_api_impl.hpp"

#include "common/hexutil.hpp"
#include "common/visitor.hpp"

namespace kagome::api {
  using primitives::BlockHash;
  using primitives::BlockNumber;

  ChainApiImpl::ChainApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : block_repo_{std::move(block_repo)}, block_tree_{std::move(block_tree)} {
    BOOST_ASSERT_MSG(block_repo_ != nullptr, "block repo parameter is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree parameter is nullptr");
  }

  outcome::result<BlockHash> ChainApiImpl::getBlockHash() const {
    auto last_finalized = block_tree_->getLastFinalized();
    return last_finalized.block_hash;
  }
  outcome::result<common::Hash256> ChainApiImpl::getBlockHash(
      BlockNumber value) const {
    return block_repo_->getHashByNumber(value);
  }

  void ChainApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<BlockHash> ChainApiImpl::getBlockHash(
      std::string_view value) const {
    // despite w3f specification says, that request contains 32-bit
    // unsigned integer, we are free to decode more capacious number,
    // since BlockNumber, which is really being requested
    // is defined as uint64_t
    OUTCOME_TRY(number, common::unhexNumber<BlockNumber>(value));
    return getBlockHash(number);
  }

  outcome::result<std::vector<BlockHash>> ChainApiImpl::getBlockHash(
      gsl::span<const ValueType> values) const {
    std::vector<BlockHash> results;
    results.reserve(values.size());

    for (const auto &v : values) {
      auto &&res = kagome::visit_in_place(
          v,
          [this](BlockNumber number) { return getBlockHash(number); },
          [this](std::string_view hex_string) {
            return getBlockHash(hex_string);
          });
      OUTCOME_TRY(r, res);
      results.emplace_back(r);
    }

    return results;
  }

  outcome::result<bool> ChainApiImpl::unsubscribeNewHeads(uint32_t id) {
    if (auto api_service = api_service_.lock())
      return api_service->unsubscribeNewHeads(id);

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<uint32_t> ChainApiImpl::subscribeNewHeads() {
    if (auto api_service = api_service_.lock())
      return api_service->subscribeNewHeads();

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

}  // namespace kagome::api
