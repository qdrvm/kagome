/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define TRY_FALSE(expr)        \
  ({                           \
    auto res = (expr);         \
    if (not res) return false; \
    std::move(res.value());    \
  })
