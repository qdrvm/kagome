/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <span>
#include <vector>

#include "parachain/types.hpp"

namespace kagome::parachain {

  struct Groups {
    std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> groups;
    std::unordered_map<ValidatorIndex, GroupIndex> by_validator_index;
    uint32_t backing_threshold;

    Groups(std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> &&g,
           uint32_t bt)
        : groups{std::move(g)}, backing_threshold{bt} {
      for (const auto &[g, vxs] : groups) {
        for (const auto &v : vxs) {
          by_validator_index[v] = g;
        }
      }
    }

    Groups(const std::vector<std::vector<ValidatorIndex>> &grs, uint32_t bt)
        : backing_threshold{bt} {
      for (GroupIndex g = 0; g < grs.size(); ++g) {
        const auto &group = grs[g];
        groups[g] = group;
        for (const auto &v : group) {
          by_validator_index[v] = g;
        }
      }
    }

    bool all_empty() const {
      for (const auto &[_, group] : groups) {
        if (!group.empty()) {
          return false;
        }
      }
      return true;
    }

    std::optional<GroupIndex> byValidatorIndex(
        ValidatorIndex validator_index) const {
      auto it = by_validator_index.find(validator_index);
      if (it != by_validator_index.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    std::optional<std::span<const ValidatorIndex>> get(
        GroupIndex group_index) const {
      auto group = groups.find(group_index);
      if (group == groups.end()) {
        return std::nullopt;
      }
      return group->second;
    }

    std::optional<std::tuple<size_t, size_t>> get_size_and_backing_threshold(
        GroupIndex group_index) const {
      auto group = get(group_index);
      if (!group) {
        return {};
      }
      return std::make_tuple(
          group->size(), std::min(group->size(), size_t(backing_threshold)));
    }
  };

}  // namespace kagome::parachain
