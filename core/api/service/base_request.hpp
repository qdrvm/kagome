/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_BASE_REQUEST_HPP
#define KAGOME_CHAIN_BASE_REQUEST_HPP

#include <jsonrpc-lean/request.h>
#include <functional>
#include <optional>
#include <tuple>

#include "api/jrpc/decode_args.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::details {
  /**
   * Base for all request classes. Automatically parses request arguments to
   * \tparam ArgumentTypes. They can be accessed in execute() via getParam<N>().
   */
  template <typename ResultType, typename... ArgumentTypes>
  struct RequestType {
   public:
    using Params = std::tuple<ArgumentTypes...>;
    using Return = ResultType;

   private:
    Params params_;

   public:
    RequestType() = default;
    virtual ~RequestType() = default;

    virtual outcome::result<ResultType> execute() = 0;

    RequestType(const RequestType &) = delete;
    RequestType &operator=(const RequestType &) = delete;

    RequestType(RequestType &&) = delete;
    RequestType &operator=(RequestType &&) = delete;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params) {
      decodeArgs(params_, params);
      return outcome::success();
    }

    template <size_t I>
    auto getParam() ->
        typename std::tuple_element<I, decltype(params_)>::type & {
      static_assert(I < std::tuple_size<decltype(params_)>::value,
                    "Incorrect index.");
      return std::get<I>(params_);
    }
  };

}  // namespace kagome::api::details

#endif  // KAGOME_CHAIN_BASE_REQUEST_HPP
