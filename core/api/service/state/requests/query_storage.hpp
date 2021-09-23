/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUESTS_QUERY_STORAGE_H
#define KAGOME_API_REQUESTS_QUERY_STORAGE_H

#include "api/service/base_request.hpp"

#include "api/service/state/state_api.hpp"

namespace kagome::api::state::request {

  class QueryStorage final
      : public details::RequestType<std::vector<StateApi::StorageChangeSet>,
                                    std::vector<std::string>,
                                    std::string,
                                    boost::optional<std::string>> {
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
      boost::optional<primitives::BlockHash> to{};
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
                                    boost::optional<std::string>> {
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
      boost::optional<primitives::BlockHash> at{};
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

#endif  // KAGOME_API_REQUESTS_QUERY_STORAGE_H
