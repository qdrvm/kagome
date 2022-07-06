#pragma once

#include <mutex>

template <typename T, typename M = std::shared_mutex>
struct SafeObject {
  template <typename... Args>
  SafeObject(Args &&... args) : t_(std::forward<Args>(args)...) {}

  template <typename F>
  inline auto exclusiveAccess(F &&f) {
    std::unique_lock lock(cs_);
    return std::forward<F>(f)(t_);
  }

  template <typename F>
  inline auto sharedAccess(F &&f) const {
    std::shared_lock lock(cs_);
    return std::forward<F>(f)(t_);
  }

 private:
  T t_;
  mutable M cs_;
};
