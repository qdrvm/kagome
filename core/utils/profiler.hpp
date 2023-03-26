#pragma once

#include "log/logger.hpp"

#include <chrono>

class TicToc {
  std::string_view name_;
  const kagome::log::Logger &log_;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_;

 public:
  TicToc(std::string_view name, const kagome::log::Logger &log)
      : name_(name), log_(log) {
    t_ = std::chrono::high_resolution_clock::now();
  }

  template <int32_t kLine = -1>
  inline void toc(int line = -1) {
    auto prev = t_;
    t_ = std::chrono::high_resolution_clock::now();
    if constexpr (kLine != -1) {
      log_->info(
          "{} at line {} lasted for {} sec",
          name_,
          std::to_string(kLine),
          std::chrono::duration_cast<std::chrono::seconds>(t_ - prev).count());
    } else {
      log_->info(
          "{} lasted for {} sec",
          name_,
          std::chrono::duration_cast<std::chrono::seconds>(t_ - prev).count());
    }
  }

  ~TicToc() {
    toc<>();
  }
};
