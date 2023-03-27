/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/chain/impl/chain_api_impl.hpp"

#include <jsonrpc-lean/fault.h>

#include "api/service/api_service.hpp"
#include "common/hexutil.hpp"
#include "common/visitor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, ChainApiImpl::Error, e) {
  using E = kagome::api::ChainApiImpl::Error;
  switch (e) {
    case E::BLOCK_NOT_FOUND:
      return "The requested block is not found";
    case E::HEADER_NOT_FOUND:
      return "The requested block header is not found";
  }
  return "Unknown Chain API error";
}

namespace kagome::api {
  using primitives::BlockHash;
  using primitives::BlockNumber;

  ChainApiImpl::ChainApiImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockStorage> block_storage,
      lazy<std::shared_ptr<api::ApiService>> api_service)
      : header_repo_{std::move(block_repo)},
        block_tree_{std::move(block_tree)},
        api_service_{std::move(api_service)},
        block_storage_{std::move(block_storage)} {
    BOOST_ASSERT_MSG(header_repo_ != nullptr,
                     "block repo parameter is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree parameter is nullptr");
    BOOST_ASSERT(block_storage_);
  }

  outcome::result<BlockHash> ChainApiImpl::getBlockHash() const {
    auto last_finalized = block_tree_->getLastFinalized();
    return last_finalized.hash;
  }
  outcome::result<common::Hash256> ChainApiImpl::getBlockHash(
      BlockNumber value) const {
    return header_repo_->getHashByNumber(value);
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

  outcome::result<primitives::BlockData> ChainApiImpl::getBlock(
      std::string_view hash) {
    OUTCOME_TRY(h, primitives::BlockHash::fromHexWithPrefix(hash));
    OUTCOME_TRY(block, block_storage_->getBlockData(h));
    if (block) {
      return block.value();
    }
    return Error::BLOCK_NOT_FOUND;
  }

  outcome::result<primitives::BlockData> ChainApiImpl::getBlock() {
    auto last_finalized_info = block_tree_->getLastFinalized();
    OUTCOME_TRY(block, block_storage_->getBlockData(last_finalized_info.hash));
    if (block) {
      return block.value();
    }
    return Error::BLOCK_NOT_FOUND;
  }

  outcome::result<primitives::BlockHash> ChainApiImpl::getFinalizedHead()
      const {
    return block_tree_->getLastFinalized().hash;
  }

  outcome::result<uint32_t> ChainApiImpl::subscribeFinalizedHeads() {
    if (auto api_service = api_service_.get()) {
      return api_service->subscribeFinalizedHeads();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<void> ChainApiImpl::unsubscribeFinalizedHeads(
      uint32_t subscription_id) {
    if (auto api_service = api_service_.get()) {
      OUTCOME_TRY(api_service->unsubscribeFinalizedHeads(subscription_id));
      // any non-error result is considered as success
      return outcome::success();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<uint32_t> ChainApiImpl::subscribeNewHeads() {
    if (auto api_service = api_service_.get()) {
      return api_service->subscribeNewHeads();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<void> ChainApiImpl::unsubscribeNewHeads(
      uint32_t subscription_id) {
    if (auto api_service = api_service_.get()) {
      OUTCOME_TRY(api_service->unsubscribeNewHeads(subscription_id));
      return outcome::success();
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

}  // namespace kagome::api
