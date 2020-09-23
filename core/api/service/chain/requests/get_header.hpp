/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_GET_HEADER_HPP
#define KAGOME_CHAIN_GET_HEADER_HPP

#include <tuple>
#include <jsonrpc-lean/request.h>
#include <boost/optional.hpp>

#include "api/service/chain/chain_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api::chain::request {

  template <size_t I1, size_t I2>
  struct is_equal {
    static constexpr bool value = I1 == I2;
  };

  template <size_t I, size_t Max, typename Tuple, typename F>
  constexpr void loop(Tuple &t, F &&f) {
    static_assert(I <= Max, "Invalid expression!");
    if constexpr (!is_equal<I, Max>::value) {
      std::forward<F>(f)(I, std::get<I>(t));
      loop<I + 1, Max>(t, std::forward<F>(f));
    }
  }

#ifdef KAGOME_LOAD_VALUE
#error Already defined!
#endif  // KAGOME_LOAD_VALUE

#define KAGOME_LOAD_VALUE(type) \
  outcome::result<void> loadValue(boost::optional<type> &dst, const jsonrpc::Value &src) { \
    type t; \
    loadValue(t, src); \
    dst = std::move(t); \
  } \
  outcome::result<void> loadValue(type &dst, const jsonrpc::Value &src)

  template <typename ResultType, typename... Types>
  struct RequestType {
    explicit RequestType(std::shared_ptr<ChainApi> &api) : api_(api){};
    virtual ~RequestType() = 0;
    virtual outcome::result<ResultType> execute() = 0;

    RequestType(const RequestType &) = delete;
    RequestType &operator=(const RequestType &) = delete;

    RequestType(RequestType &&) = delete;
    RequestType &operator=(RequestType &&) = delete;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params) {
      if (params.size() <= sizeof...(Types)) {
        loop<0, sizeof...(Types)>(params_, [&](const auto ix, auto &dst) {
          if (ix < params.size()) loadValue(dst, params[ix]);
        });
        return outcome::success();
      }
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }

   private:
    std::tuple<Types...> params_;
    std::shared_ptr<ChainApi> api_;

    KAGOME_LOAD_VALUE(int32_t) {
      if (!src.IsInteger32())
        throw jsonrpc::InvalidParametersFault("invalid argument");

      dst = src.AsInteger32();
      return outcome::success();
    };

    KAGOME_LOAD_VALUE(std::string) {
      if (!src.IsString())
        throw jsonrpc::InvalidParametersFault("invalid argument");

      dst = src.AsString();
      return outcome::success();
    };
  };
#undef KAGOME_LOAD_VALUE

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_GET_HEADER_HPP
