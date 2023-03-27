/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_API_IMPL_HPP
#define KAGOME_CHAIN_API_IMPL_HPP

#include "api/service/chain/chain_api.hpp"

#include <memory>

#include <boost/di/extension/injections/lazy.hpp>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"

namespace kagome::api {

  template <typename T>
  using lazy = boost::di::extension::lazy<T>;

  class ChainApiImpl : public ChainApi {
   public:
    enum class Error {
      BLOCK_NOT_FOUND = 1,
      HEADER_NOT_FOUND,
    };

    ~ChainApiImpl() override = default;

    ChainApiImpl(std::shared_ptr<blockchain::BlockHeaderRepository> block_repo,
                 std::shared_ptr<blockchain::BlockTree> block_tree,
                 std::shared_ptr<blockchain::BlockStorage> block_storage,
                 lazy<std::shared_ptr<api::ApiService>> api_service);

    outcome::result<BlockHash> getBlockHash() const override;

    outcome::result<BlockHash> getBlockHash(BlockNumber value) const override;

    outcome::result<BlockHash> getBlockHash(
        std::string_view value) const override;

    outcome::result<std::vector<BlockHash>> getBlockHash(
        gsl::span<const ValueType> values) const override;

    outcome::result<primitives::BlockHeader> getHeader(
        std::string_view hash) override {
      OUTCOME_TRY(h, primitives::BlockHash::fromHexWithPrefix(hash));
      return header_repo_->getBlockHeader(h);
    }

    outcome::result<primitives::BlockHeader> getHeader() override {
      auto last = block_tree_->getLastFinalized();
      return header_repo_->getBlockHeader(last.hash);
    }

    outcome::result<primitives::BlockData> getBlock(
        std::string_view hash) override;
    outcome::result<primitives::BlockData> getBlock() override;

    outcome::result<primitives::BlockHash> getFinalizedHead() const override;

    outcome::result<uint32_t> subscribeFinalizedHeads() override;
    outcome::result<void> unsubscribeFinalizedHeads(
        uint32_t subscription_id) override;

    outcome::result<uint32_t> subscribeNewHeads() override;
    outcome::result<void> unsubscribeNewHeads(
        uint32_t subscription_id) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    lazy<std::shared_ptr<api::ApiService>> api_service_;
    std::shared_ptr<blockchain::BlockStorage> block_storage_;
  };
}  // namespace kagome::api

OUTCOME_HPP_DECLARE_ERROR(kagome::api, ChainApiImpl::Error);

#endif  // KAGOME_CHAIN_API_IMPL_HPP
