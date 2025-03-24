/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#define CUSTOM_EQUALITY(Self, ...)                    \
  decltype(auto) _tie() const {                       \
    return std::tie(__VA_ARGS__);                     \
  }                                                   \
  bool operator==(const Self &other) const noexcept { \
    return _tie() == other._tie();                    \
  }
