/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstring>
#include <qtils/cxx20/lexicographical_compare_three_way.hpp>
#include <span>

// `std::span` doesn't have comparison operator functions.
// Can't add function to neither `std::span` nor `namespace std`.
// `SpanAdl{span}` wraps span and allows writing functions for `SpanAdl`.
// `SpanAdl` overload will be selected by ADL.
template <typename T>
struct SpanAdl {
  std::span<T> v;
};
template <typename T>
SpanAdl(T &&t) -> SpanAdl<typename decltype(std::span{t})::element_type>;

template <typename T>
auto operator<=>(const SpanAdl<T> &l_, const auto &r_)
  requires(requires { std::span<const T>{r_}; })
{
  auto &[l] = l_;
  std::span r{r_};
  if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint8_t>) {
    auto n = std::min(l.size(), r.size());
    auto c = std::memcmp(l.data(), r.data(), n) <=> 0;
    return c != 0 ? c : l.size() <=> r.size();
  } else {
    return qtils::cxx20::lexicographical_compare_three_way(
        l.begin(), l.end(), r.begin(), r.end());
  }
}

template <typename T>
bool operator==(const SpanAdl<T> &l_, const auto &r_)
  requires(requires { std::span<const T>{r_}; })
{
  std::span r{r_};
  return l_.v.size() == r.size() and (l_ <=> r) == 0;
}
