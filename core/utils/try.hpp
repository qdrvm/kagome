/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define TRY_FALSE(expr)      \
  ({                         \
    auto r = (expr);         \
    if (not r) return false; \
    std::move(r.value());    \
  })
