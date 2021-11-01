/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_BASE_REQUEST_HPP
#define KAGOME_CHAIN_BASE_REQUEST_HPP

#include <jsonrpc-lean/request.h>
#include <functional>
#include <optional>
#include <tuple>

#include "outcome/outcome.hpp"

namespace kagome::api::details {

  template <size_t I, size_t Max, typename Tuple, typename F>
  constexpr void loop(Tuple &t, F &&f) {
    static_assert(I <= Max, "Invalid expression!");
    if constexpr (!std::equal_to<size_t>()(I, Max)) {
      std::forward<F>(f)(I, std::get<I>(t));
      loop<I + 1, Max>(t, std::forward<F>(f));
    }
  }

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
      if (params.size() <= sizeof...(ArgumentTypes)) {
        loop<0, sizeof...(ArgumentTypes)>(params_,
                                          [&](const auto ix, auto &dst) {
                                            if (ix < params.size())
                                              loadValue(dst, params[ix]);
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
    template <typename T>
    void loadValue(std::optional<T> &dst, const jsonrpc::Value &src) {
      if (!src.IsNil()) {
        T t;
        loadValue(t, src);
        dst = std::move(t);
      } else {
        dst = std::nullopt;
      }
    }

    template <typename SequenceContainer,
              typename = typename SequenceContainer::value_type,
              typename = typename SequenceContainer::iterator>
    void loadValue(SequenceContainer &dst, const jsonrpc::Value &src) {
      if (!src.IsNil() && src.IsArray()) {
        for (auto &v : src.AsArray()) {
          typename SequenceContainer::value_type t;
          loadValue(t, v);
          dst.insert(dst.end(), std::move(t));
        }
      } else {
        throw jsonrpc::InvalidParametersFault("invalid argument type");
      }
    }

    template <typename T>
    std::enable_if_t<std::is_integral_v<T>, void> loadValue(
        T &dst, const jsonrpc::Value &src) {
      if (not src.IsInteger32() and not src.IsInteger64()) {
        throw jsonrpc::InvalidParametersFault("invalid argument type");
      }
      auto num = src.AsInteger64();
      if constexpr (std::is_signed_v<T>) {
        if (num < std::numeric_limits<T>::min()
            or num > std::numeric_limits<T>::max()) {
          throw jsonrpc::InvalidParametersFault("invalid argument value");
        }
      } else {
        if (num < 0
            or static_cast<uint64_t>(num) > std::numeric_limits<T>::max()) {
          throw jsonrpc::InvalidParametersFault("invalid argument value");
        }
      }
      dst = num;
    }

    void loadValue(bool &dst, const jsonrpc::Value &src) {
      if (not src.IsBoolean()) {
        throw jsonrpc::InvalidParametersFault("invalid argument type");
      }
      dst = src.AsBoolean();
    }

    void loadValue(std::string &dst, const jsonrpc::Value &src) {
      if (not src.IsString()) {
        throw jsonrpc::InvalidParametersFault("invalid argument type");
      }
      dst = src.AsString();
    }
  };

}  // namespace kagome::api::details

#endif  // KAGOME_CHAIN_BASE_REQUEST_HPP
