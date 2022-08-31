#pragma once

class NonCopyable {
 public:
  NonCopyable(NonCopyable const &) = delete;
  NonCopyable &operator=(NonCopyable const &) = delete;

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
