/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include <qtils/tagged.hpp>

template <typename T, typename Tag>
struct fmt::formatter<qtils::Tagged<T, Tag>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const qtils::Tagged<T, Tag> &tagged, FormatContext &ctx) {
    return fmt::formatter<T>::format(untagged(tagged), ctx);
  }
};
