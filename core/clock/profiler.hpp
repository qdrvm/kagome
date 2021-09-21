#pragma once

#include "log/logger.hpp"

#include <chrono>

class TicToc {
  std::string name_;
  const kagome::log::Logger &log_;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_;

 public:
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
    log_->warn("{} lasted for {} sec",
               str,
               std::chrono::duration_cast<std::chrono::seconds>(t_ - prev)
                   .count());
  }

  ~TicToc() {
    toc();
  }
};
