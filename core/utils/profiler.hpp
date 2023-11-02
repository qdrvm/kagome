/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

#include <chrono>

class TicToc {
  std::string_view name_;
  const kagome::log::Logger &log_;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_;

 public:
  TicToc(std::string &&) = delete;
  TicToc(const std::string &) = delete;
  TicToc(std::string_view name, const kagome::log::Logger &log)
      : name_(name), log_(log) {
    t_ = std::chrono::high_resolution_clock::now();
  }

  void toc() {
    auto prev = t_;
    t_ = std::chrono::high_resolution_clock::now();
    log_->info("{} lasted for {} ms",
               name_,
               std::chrono::duration_cast<std::chrono::milliseconds>(t_ - prev)
                   .count());
  }

  void toc(int line) {
    auto prev = t_;
    t_ = std::chrono::high_resolution_clock::now();
    log_->info("{} at line {} lasted for {} ms",
               name_,
               std::to_string(line),
               std::chrono::duration_cast<std::chrono::milliseconds>(t_ - prev)
                   .count());
  }

  ~TicToc() {
    toc();
  }
};
