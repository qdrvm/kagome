/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/chain/impl/chain_api_impl.hpp"

#include <boost/algorithm/hex.hpp>
#include "common/hexutil.hpp"
#include "common/visitor.hpp"

namespace kagome::api {
  using kagome::primitives::BlockHash;
  namespace {
    outcome::result<uint32_t> unhexNumber(std::string_view value) {
      if (value.substr(0, 2) == "0x") {
        return unhexNumber(value.substr(2));
      }
      OUTCOME_TRY(bytes, common::unhex(value));
      uint32_t result = 0;
      for (auto b : bytes) {
        result *= 10;
        result += b;
      }

      return result;
    }
  }  // namespace

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
      uint32_t value) const {
    return block_repo_->getHashByNumber(value);
  }

  outcome::result<BlockHash> ChainApiImpl::getBlockHash(
      std::string_view value) const {
    OUTCOME_TRY(number, unhexNumber(value));
    return getBlockHash(number);
  }

  outcome::result<std::vector<BlockHash>> ChainApiImpl::getBlockHash(
      gsl::span<const ValueType> values) const {
    std::vector<BlockHash> results;
    results.reserve(values.size());

    for (const auto &v : values) {
      auto &&res = kagome::visit_in_place(
          v,
          [this](const uint32_t &number) { return getBlockHash(number); },
          [this](const std::string_view &hex) { return getBlockHash(hex); });
      OUTCOME_TRY(r, res);
      results.emplace_back(r);
    }

    return results;
  }

}  // namespace kagome::api

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, ChainApiError, e) {
  using kagome::api::ChainApiError;
  switch (e) {
    case ChainApiError::INVALID_ARGUMENT:
      return "invalid argument";
  }
  return "unknown ChainApiError";
}
