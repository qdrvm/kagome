/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <type_traits>  // for std::decay
#include <utility>      // for std::forward
#include <variant>

#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>  // for boost::apply_visitor

#include "common/tagged.hpp"

namespace kagome {

  template <typename... Lambdas>
  struct lambda_visitor;

  template <typename Lambda, typename... Lambdas>
  struct lambda_visitor<Lambda, Lambdas...>
      : public Lambda, public lambda_visitor<Lambdas...> {
    using Lambda::operator();
    using lambda_visitor<Lambdas...>::operator();

    // NOLINTNEXTLINE(google-explicit-constructor)
    lambda_visitor(Lambda lambda, Lambdas... lambdas)
        : Lambda(lambda), lambda_visitor<Lambdas...>(lambdas...) {}
  };

  template <typename Lambda>
  struct lambda_visitor<Lambda> : public Lambda {
    using Lambda::operator();

    // NOLINTNEXTLINE(google-explicit-constructor)
    lambda_visitor(Lambda lambda) : Lambda(lambda) {}
  };

  /**
   * @brief Convenient in-place compile-time visitor creation, from a set of
   * lambdas
   *
   * @code
   * make_visitor([](int a){ return 1; },
   *              [](std::string b) { return 2; });
   * @nocode
   *
   * is essentially the same as
   *
   * @code
   * struct visitor : public boost::static_visitor<int> {
   *   int operator()(int a) { return 1; }
   *   int operator()(std::string b) { return 2; }
   * }
   * @nocode
   *
   * @param lambdas
   * @return visitor
   */
  template <class... Fs>
  constexpr auto make_visitor(Fs &&...fs) {
    using visitor_type = lambda_visitor<std::decay_t<Fs>...>;
    return visitor_type(std::forward<Fs>(fs)...);
  }

  template <typename... Ts>
  struct is_std_variant : std::false_type {};

  template <typename... Ts>
  struct is_std_variant<std::variant<Ts...>> : std::true_type {};

  /**
   * @brief Inplace visitor for boost::variant.
   * @code
   *   boost::variant<int, std::string> value = "1234";
   *   ...
   *   visit_in_place(value,
   *                  [](int v) { std::cout << "(int)" << v; },
   *                  [](std::string v) { std::cout << "(string)" << v;}
   *                  );
   * @nocode
   *
   * @param variant
   * @param lambdas
   */
  template <
      typename TVariant,
      std::enable_if_t<is_std_variant<std::decay_t<TVariant>>::value == false,
                       bool> = true,
      typename... TVisitors>
  constexpr decltype(auto) visit_in_place(TVariant &&variant,
                                          TVisitors &&...visitors) {
    return boost::apply_visitor(
        make_visitor(std::forward<TVisitors>(visitors)...),
        std::forward<TVariant>(variant));
  }

  /**
   * @brief Inplace visitor for std::variant.
   * @code
   *   std::variant<int, std::string> value = "1234";
   *   ...
   *   visit_in_place(value,
   *                  [](int v) { std::cout << "(int)" << v; },
   *                  [](std::string v) { std::cout << "(string)" << v;}
   *                  );
   * @nocode
   *
   * @param variant
   * @param lambdas
   */
  template <
      typename TVariant,
      std::enable_if_t<is_std_variant<std::decay_t<TVariant>>::value == true,
                       bool> = true,
      typename... TVisitors>
  constexpr decltype(auto) visit_in_place(TVariant &&variant,
                                          TVisitors &&...visitors) {
    return std::visit(make_visitor(std::forward<TVisitors>(visitors)...),
                      std::forward<TVariant>(variant));
  }

  template <typename TReturn, typename TVariant>
  constexpr std::optional<std::reference_wrapper<TReturn>> if_type(
      TVariant &&variant) {
    if (auto ptr = boost::get<TReturn>(&variant)) {
      return *ptr;
    }
    return std::nullopt;
  }

  template <typename Type, typename TVariant>
  constexpr bool is_type(TVariant &&variant) {
    return boost::get<Type>(&variant) != nullptr;
  }

  template <typename T, typename Tag, typename TVariant>
  constexpr bool is_tagged_by(TVariant &&variant) {
    return is_type<Tagged<T, Tag>>(&variant);
  }

  /// apply Matcher to optional T
  template <typename T, typename Matcher>
  constexpr decltype(auto) match(T &&t, Matcher &&m) {
    return std::forward<T>(t) ? std::forward<Matcher>(m)(*std::forward<T>(t))
                              : std::forward<Matcher>(m)();
  }

  /// construct visitor from Fs and apply it to optional T
  template <typename T, typename... Fs>
  constexpr decltype(auto) match_in_place(T &&t, Fs &&...fs) {
    return match(std::forward<T>(t), make_visitor(std::forward<Fs>(fs)...));
  }
}  // namespace kagome::common
