/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/impl/system_api_impl.hpp"

#include <queue>

#include <jsonrpc-lean/request.h>

#include "blockchain/block_tree.hpp"
#include "primitives/ss58_codec.hpp"
#include "scale/scale.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {

  SystemApiImpl::SystemApiImpl(
      std::shared_ptr<application::ChainSpec> config,
      std::shared_ptr<consensus::babe::Babe> babe,
      std::shared_ptr<network::PeerManager> peer_manager,
      std::shared_ptr<runtime::AccountNonceApi> account_nonce_api,
      std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
      std::shared_ptr<const blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Hasher> hasher)
      : config_(std::move(config)),
        babe_(std::move(babe)),
        peer_manager_(std::move(peer_manager)),
        account_nonce_api_(std::move(account_nonce_api)),
        transaction_pool_(std::move(transaction_pool)),
        block_tree_(std::move(block_tree)),
        hasher_{std::move(hasher)} {
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(babe_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(account_nonce_api_ != nullptr);
    BOOST_ASSERT(transaction_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
  }

  std::shared_ptr<application::ChainSpec> SystemApiImpl::getConfig() const {
    return config_;
  }

  std::shared_ptr<consensus::babe::Babe> SystemApiImpl::getBabe() const {
    return babe_;
  }

  std::shared_ptr<network::PeerManager> SystemApiImpl::getPeerManager() const {
    return peer_manager_;
  }

  outcome::result<primitives::AccountNonce> SystemApiImpl::getNonceFor(
      std::string_view account_address) const {
    OUTCOME_TRY(account_id, primitives::decodeSs58(account_address, *hasher_));
    OUTCOME_TRY(nonce,
                account_nonce_api_->account_nonce(block_tree_->bestBlock().hash,
                                                  account_id));

    return adjustNonce(account_id, nonce);
  }

  primitives::AccountNonce SystemApiImpl::adjustNonce(
      const primitives::AccountId &account_id,
      primitives::AccountNonce current_nonce) const {
    std::vector<std::pair<primitives::Transaction::Hash,
                          std::shared_ptr<const primitives::Transaction>>>
        txs = transaction_pool_->getReadyTransactions();

    // txs authored by the provided account sorted by nonce
    std::map<primitives::AccountNonce,
             std::reference_wrapper<const primitives::Transaction>>
        sorted_txs;

    for (const auto &[_, tx_ptr] : txs) {
      if (tx_ptr->provided_tags.empty()) {
        continue;
      }

      // the assumption that the tag with nonce is encoded this way is taken
      // from substrate
      auto tag_decode_res = scale::decode<
          std::tuple<primitives::AccountId, primitives::AccountNonce>>(
          tx_ptr->provided_tags.at(0));  // substrate assumes that the tag with
                                         // nonce is the first one

      if (tag_decode_res.has_value()) {
        auto &&[id, nonce] = std::move(tag_decode_res.value());
        if (id == account_id) {
          sorted_txs.emplace(nonce, std::ref(*tx_ptr));
        }
      }
    }
    // ensure that we consider only continuously growing nonce
    for (auto &&[tx_nonce, tx] : sorted_txs) {
      if (tx_nonce == current_nonce) {
        current_nonce++;
      } else {
        break;
      }
    }

    return current_nonce;
  }

}  // namespace kagome::api
