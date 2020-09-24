/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_BASE_REQUEST_HPP
#define KAGOME_CHAIN_BASE_REQUEST_HPP

#include <jsonrpc-lean/request.h>
#include <boost/optional.hpp>
#include <functional>
#include <tuple>

#include "api/service/chain/chain_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api::chain::request {

  template <size_t I, size_t Max, typename Tuple, typename F>
  constexpr void loop(Tuple &t, F &&f) {
    static_assert(I <= Max, "Invalid expression!");
    if constexpr (!std::equal_to<size_t>()(I, Max)) {
      std::forward<F>(f)(I, std::get<I>(t));
      loop<I + 1, Max>(t, std::forward<F>(f));
    }
  }

#pragma push_macro("KAGOME_LOAD_VALUE")
#undef KAGOME_LOAD_VALUE

#define KAGOME_LOAD_VALUE(type)                                           \
  void loadValue(boost::optional<type> &dst, const jsonrpc::Value &src) { \
    if (!src.IsNil()) {                                                  \
      type t;                                                             \
      loadValue(t, src);                                                  \
      dst = std::move(t);                                                 \
    } else {                                                              \
      dst = boost::none;                                                  \
    }                                                                     \
  }                                                                       \
  void loadValue(type &dst, const jsonrpc::Value &src)

  template <typename ResultType, typename... Types>
  struct RequestType {
   private:
    std::tuple<Types...> params_;

   public:
    RequestType() = default;
    virtual ~RequestType() = default;

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

    template <size_t I>
    auto getParam() ->
        typename std::tuple_element<I, decltype(params_)>::type & {
      static_assert(I < std::tuple_size<decltype(params_)>::value,
                    "Incorrect index.");
      return std::get<I>(params_);
    }

   private:
    KAGOME_LOAD_VALUE(int32_t) {
      if (!src.IsInteger32())
        throw jsonrpc::InvalidParametersFault("invalid argument");

      dst = src.AsInteger32();
    }

    KAGOME_LOAD_VALUE(std::string) {
      if (!src.IsString())
        throw jsonrpc::InvalidParametersFault("invalid argument");

      dst = src.AsString();
    }
  };
#pragma pop_macro("KAGOME_LOAD_VALUE")

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_BASE_REQUEST_HPP
