#pragma once

#include "log/logger.hpp"
#include "log/profiling_logger.hpp"

#include <chrono>

namespace {

  template <typename T>
  constexpr std::string unit() {
    if constexpr (std::is_same_v<T, std::chrono::seconds>) {
      return "sec";
    } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
      return "ms";
    }
    return "";
  }

}  // namespace

template <typename T = std::chrono::milliseconds>
class TicToc {
  std::string name_;
  const kagome::log::Logger &log_;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_;

 public:
  TicToc() : TicToc("") {}

  TicToc(const std::string &name)
      : TicToc(name, ::kagome::log::profiling_logger) {}

  TicToc(const std::string &name, const kagome::log::Logger &log)
      : name_(name), log_(log) {
    t_ = std::chrono::high_resolution_clock::now();
  }

  void toc(int line = -1) {
    auto prev = t_;
    t_ = std::chrono::high_resolution_clock::now();
    auto str = name_;
    if (line != -1) {
      str += "at line " + std::to_string(line);
    }
    log_->warn("{} lasted for {} {}",
                  str,
                  std::chrono::duration_cast<T>(t_ - prev).count(),
                  unit<T>());
  }

  ~TicToc() {
    toc();
  }
};
