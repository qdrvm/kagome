/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <boost/range/adaptor/indexed.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "crypto/chacha.hpp"
#include "crypto/hasher/hasher_impl.hpp"

namespace kagome::parachain::grid {
  /// The sample rate for randomly propagating messages. This
  /// reduces the left tail of the binomial distribution but also
  /// introduces a bias towards peers who we sample before others
  /// (i.e. those who get a block before others).
  constexpr size_t DEFAULT_RANDOM_SAMPLE_RATE = 25;

  /// The number of peers to randomly propagate messages to.
  constexpr size_t DEFAULT_RANDOM_CIRCULATION = 4;

  /**
   * Numbers arranged into rectangular grid.
   * https://github.com/paritytech/polkadot-sdk/blob/2aaa9af3746b0cf671de9dc98fe2465c7ef59be2/polkadot/node/network/protocol/src/grid_topology.rs#L69-L149
   */
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

    bool operator==(const View &r) const {
      return sending == r.sending && receiving == r.receiving;
    }

    bool canReceive(bool full, ValidatorIndex from) const {
      return (full ? receiving : sending).contains(from);
    }

    void sendTo(bool full, auto &&f) const {
      if (full) {
        for (auto &i : sending) {
          // https://github.com/paritytech/polkadot-sdk/blob/d5fe478e4fe2d62b0800888ae77b00ff0ba28b28/polkadot/node/network/statement-distribution/src/v2/grid.rs#L420-L435
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

  /// Routing mode
  struct RequiredRouting {
    enum {  // NOLINT(performance-enum-size)
      /// We don't know yet, because we're waiting for topology info
      /// (race condition between learning about the first blocks in a new
      /// session
      /// and getting the topology for that session)
      PendingTopology,
      /// Propagate to all peers of any kind.
      All,
      /// Propagate to all peers sharing either the X or Y dimension of the
      /// grid.
      GridXY,
      /// Propagate to all peers sharing the X dimension of the grid.
      GridX,
      /// Propagate to all peers sharing the Y dimension of the grid.
      GridY,
      /// No required propagation.
      None
    } value;

    /// Whether the required routing set is definitely empty.
    bool is_empty() const {
      return value == PendingTopology || value == None;
    }

    bool operator==(const RequiredRouting &l) const {
      return value == l.value;
    }
  };

  /// A representation of routing based on sample
  struct RandomRouting {
    /// The number of peers to target.
    size_t target;
    /// The number of peers this has been sent to.
    size_t sent;
    /// Sampling rate
    size_t sample_rate;

    RandomRouting()
        : target(DEFAULT_RANDOM_CIRCULATION),
          sent(0),
          sample_rate(DEFAULT_RANDOM_SAMPLE_RATE) {}

    /// Perform random sampling for a specific peer
    /// Returns `true` for a lucky peer
    bool sample(size_t n_peers_total) const {
      if (n_peers_total == 0 || sent >= target) {
        return false;
      } else if (sample_rate > n_peers_total) {
        return true;
      } else {
        std::srand(std::time(nullptr));
        size_t random_number = (std::rand() % n_peers_total) + size_t(1);
        return random_number <= sample_rate;
      }
    }

    /// Increase number of messages being sent
    void inc_sent() {
      sent += 1;
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
    for (auto [i, v] : boost::adaptors::index(validators)) {
      index[v] = i;
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
      size_t n,
      std::span<const uint8_t, 32> babe_randomness) {
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
