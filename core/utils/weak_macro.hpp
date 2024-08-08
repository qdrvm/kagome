/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define WEAK_SELF    \
  weak_self {        \
    weak_from_this() \
  }

#define WEAK_LOCK(name)           \
  auto name = weak_##name.lock(); \
  if (not name) return;

#define IF_WEAK_LOCK(name) if (auto name = weak_##name.lock())
