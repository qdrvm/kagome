/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_JRPC_JRPC_METHOD_HPP
#define KAGOME_CORE_API_JRPC_JRPC_METHOD_HPP

#include <jsonrpc-lean/dispatcher.h>
#include <jsonrpc-lean/request.h>

#include <memory>
#include <type_traits>

#include "api/jrpc/value_converter.hpp"

namespace kagome::api {

  template <typename RequestType, typename Api>
  class Method {
   private:
    std::weak_ptr<Api> api_;

   public:
    Method(const std::shared_ptr<Api> &api) : api_(api) {}

    jsonrpc::Value operator()(const jsonrpc::Request::Parameters &params) {
      if (auto api = api_.lock()) {
        RequestType request(api);

        if (auto &&resust = request.init(params); not resust) {
          throw jsonrpc::Fault(resust.error().message());
        }

        if (auto &&result = request.execute(); not result) {
          throw jsonrpc::Fault(result.error().message());
        } else if constexpr (std::is_same_v<decltype(result.value()),
                                            void>) {  // NOLINT
          return {};
        } else {  // NOLINT
          return makeValue(result.value());
        }

      } else {
        throw jsonrpc::Fault("API not available");
      }
    }
  };

}  // namespace kagome::api

#endif // KAGOME_CORE_API_JRPC_JRPC_METHOD_HPP
