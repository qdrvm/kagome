/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "crypto/hasher/hasher_impl.hpp"
#include "parachain/backing/chacha.hpp"

namespace kagome::parachain::grid {
  struct Grid {
    Grid(size_t count) : count{count}, width(std::sqrt(count)) {}

    void cross(size_t center, auto &&f) const {
      auto c = _split(center);
      _vertical(center, c.second, f);
      _horizontal(center, c.first, f);
    }

    void project(size_t center, size_t other, auto &&f) const {
      [[unlikely]] if (center == other) { return; }
      auto c = _split(center), o = _split(other);
      [[unlikely]] if (c.first == o.first || c.second == o.second) {
        f(other);
        return;
      }
      if (auto i = c.first + o.second; i < count) {
        f(i);
      }
      if (auto i = o.first + c.second; i < count) {
        f(i);
      }
    }

    bool orthogonal(size_t center, size_t other, auto &&f) const {
      [[unlikely]] if (center == other) { return false; }
      auto c = _split(center), o = _split(other);
      if (c.first == o.first) {
        _vertical(center, c.second, f);
        return true;
      }
      if (c.second == o.second) {
        _horizontal(center, c.first, f);
        return true;
      }
      return false;
    }

    std::pair<size_t, size_t> _split(size_t i) const {
      [[unlikely]] if (i > count) { throw std::range_error{"Grid"}; }
      auto x = i % width;
      return {i - x, x};
    }

    void _vertical(size_t center, size_t x, auto &&f) const {
      for (size_t i = x; i < count; i += width) {
        if (i != center) {
          f(i);
        }
      }
    }

    void _horizontal(size_t center, size_t min_x, auto &&f) const {
      auto max_x = std::min(min_x + width, count);
      for (size_t i = min_x; i != max_x; ++i) {
        if (i != center) {
          f(i);
        }
      }
    }

    size_t count, width;
  };

  using ValidatorIndex = uint32_t;

  /// View for one group
  struct View {
    std::unordered_set<ValidatorIndex> receiving, sending;

    bool canReceive(bool full, ValidatorIndex from) const {
      return (full ? receiving : sending).contains(from);
    }

    void sendTo(bool full, auto &&f) const {
      if (full) {
        for (auto &i : sending) {
          if (not receiving.contains(i)) {
            f(i);
          }
        }
      } else {
        for (auto &i : receiving) {
          f(i);
        }
      }
    }
  };

  /// View for each group
  using Views = std::vector<View>;

  /// Make view for each group
  inline Views makeViews(const std::vector<std::vector<ValidatorIndex>> &groups,
                         const std::vector<ValidatorIndex> &validators,
                         ValidatorIndex center) {
    Views views;
    views.reserve(groups.size());
    std::vector<size_t> index;
    index.resize(validators.size());
    for (size_t i = 0; auto &v : validators) {
      index[v] = i++;
    }
    auto as_value = [&](auto &&f) {
      return [&, f{std::forward<decltype(f)>(f)}](size_t i) mutable {
        f(validators[i]);
      };
    };
    Grid grid{validators.size()};
    for (auto &group : groups) {
      auto in_group = [&](ValidatorIndex v) {
        return std::find(group.begin(), group.end(), v) != group.end();
      };
      auto &view = views.emplace_back();
      if (in_group(center)) {
        grid.cross(index[center], as_value([&](ValidatorIndex v) {
                     if (not in_group(v)) {
                       view.sending.emplace(v);
                     }
                   }));
        continue;
      }
      for (auto &v : group) {
        grid.project(index[center], index[v], as_value([&](ValidatorIndex v) {
                       view.receiving.emplace(v);
                     }));
        grid.orthogonal(
            index[center], index[v], as_value([&](ValidatorIndex v) {
              if (not in_group(v)) {
                view.sending.emplace(v);
              }
            }));
      }
    }
    return views;
  }

  inline std::vector<ValidatorIndex> shuffle(
      const std::vector<std::vector<ValidatorIndex>> &groups,
      std::span<const uint8_t, 32> babe_randomness) {
    size_t n = 0;
    for (auto &group : groups) {
      n += group.size();
    }
    std::vector<ValidatorIndex> validators;
    validators.resize(n);
    std::iota(validators.begin(), validators.end(), 0);
    std::array<uint8_t, 8 + 32> subject;
    memcpy(subject.data(), "gossipsu", 8);
    memcpy(subject.data() + 8, babe_randomness.data(), 32);
    auto seed = crypto::HasherImpl{}.blake2b_256(subject);
    crypto::RandChaCha20{seed}.shuffle(validators);
    return validators;
  }
}  // namespace kagome::parachain::grid
