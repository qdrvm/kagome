/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

class NonCopyable {
 public:
  NonCopyable(const NonCopyable &) = delete;
  NonCopyable &operator=(const NonCopyable &) = delete;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

class NonMovable {
 public:
  NonMovable(NonMovable &&) = delete;
  NonMovable &operator=(NonMovable &&) = delete;

 protected:
  NonMovable() = default;
  ~NonMovable() = default;
};
