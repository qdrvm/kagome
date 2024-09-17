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
  NonCopyable(NonCopyable &&) = default;
  NonCopyable &operator=(NonCopyable &&) = default;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

class NonMovable {
 public:
  NonMovable(const NonMovable &) = default;
  NonMovable(NonMovable &&) = delete;
  NonMovable &operator=(const NonMovable &) = default;
  NonMovable &operator=(NonMovable &&) = delete;

 protected:
  NonMovable() = default;
  ~NonMovable() = default;
};
