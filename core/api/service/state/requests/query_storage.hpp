/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/base_request.hpp"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include "api/jrpc/value_converter.hpp"
#include "api/service/state/state_api.hpp"

namespace kagome::api {

  inline jsonrpc::Value makeValue(const StateApi::StorageChangeSet &changes) {
    jsonrpc::Value ret;
    std::vector<jsonrpc::Value::Array> j_changes;
    boost::range::push_back(
        j_changes,
        changes.changes | boost::adaptors::transformed([](auto &change) {
          return jsonrpc::Value::Array{makeValue(change.key),
                                       makeValue(change.data)};
        }));

    return jsonrpc::Value::Struct{
        std::pair{"block", makeValue(common::hex_lower_0x(changes.block))},
        std::pair{"changes", makeValue(j_changes)}};
  }

}  // namespace kagome::api

namespace kagome::api::state::request {

  class QueryStorage final
      : public details::RequestType<std::vector<StateApi::StorageChangeSet>,
                                    std::vector<std::string>,
                                    std::string,
                                    std::optional<std::string>> {
   public:
    explicit QueryStorage(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<std::vector<StateApi::StorageChangeSet>> execute()
        override {
      std::vector<common::Buffer> keys;
      keys.reserve(getParam<0>().size());
      for (auto &str_key : getParam<0>()) {
        OUTCOME_TRY(key, kagome::common::unhexWith0x(str_key));
        keys.emplace_back(std::move(key));
      }
      OUTCOME_TRY(from,
                  primitives::BlockHash::fromHexWithPrefix(getParam<1>()));
      std::optional<primitives::BlockHash> to{};
      if (auto opt_to = getParam<2>(); opt_to.has_value()) {
        OUTCOME_TRY(to_,
                    primitives::BlockHash::fromHexWithPrefix(opt_to.value()));
        to = std::move(to_);
      }
      return api_->queryStorage(keys, from, std::move(to));
    }

   private:
    std::shared_ptr<StateApi> api_;
  };

  class QueryStorageAt final
      : public details::RequestType<std::vector<StateApi::StorageChangeSet>,
                                    std::vector<std::string>,
                                    std::optional<std::string>> {
   public:
    explicit QueryStorageAt(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    }

    outcome::result<std::vector<StateApi::StorageChangeSet>> execute()
        override {
      std::vector<common::Buffer> keys;
      keys.reserve(getParam<0>().size());
      for (auto &str_key : getParam<0>()) {
        OUTCOME_TRY(key, common::unhexWith0x(str_key));
        keys.emplace_back(std::move(key));
      }
      std::optional<primitives::BlockHash> at{};
      if (auto opt_at = getParam<1>(); opt_at.has_value()) {
        OUTCOME_TRY(at_,
                    primitives::BlockHash::fromHexWithPrefix(opt_at.value()));
        at = std::move(at_);
      }

      return api_->queryStorageAt(keys, std::move(at));
    }

   private:
    std::shared_ptr<StateApi> api_;
  };

}  // namespace kagome::api::state::request
