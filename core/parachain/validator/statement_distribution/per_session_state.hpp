
/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_map>

#include <libp2p/peer/peer_id.hpp>
#include "parachain/groups.hpp"
#include "parachain/types.hpp"
#include "utils/safe_object.hpp"

namespace kagome::parachain::statement_distribution {

  using LocalValidatorIndex = std::optional<ValidatorIndex>;
  using PeerUseCount =
      SafeObject<std::unordered_map<primitives::AuthorityDiscoveryId, size_t>>;

  struct PerSessionState final {
    PerSessionState(const PerSessionState &) = delete;
    PerSessionState &operator=(const PerSessionState &) = delete;

    PerSessionState(PerSessionState &&) = default;
    PerSessionState &operator=(PerSessionState &&) = delete;

    SessionIndex session;
    runtime::SessionInfo session_info;
    Groups groups;
    std::optional<grid::Views> grid_view;
    LocalValidatorIndex local_validator;
    std::optional<ValidatorIndex> our_index;
    std::optional<GroupIndex> our_group;
    std::shared_ptr<PeerUseCount> peers;
    std::unordered_map<primitives::AuthorityDiscoveryId, ValidatorIndex>
        authority_lookup;

    PerSessionState(SessionIndex _session,
                    runtime::SessionInfo _session_info,
                    Groups &&_groups,
                    grid::Views &&_grid_view,
                    std::optional<ValidatorIndex> _our_index,
                    std::shared_ptr<PeerUseCount> _peers)
        : session{_session},
          session_info{std::move(_session_info)},
          groups{std::move(_groups)},
          grid_view{std::move(_grid_view)},
          our_index{_our_index},
          peers(std::move(_peers)) {
      if (our_index) {
        our_group = groups.byValidatorIndex(*our_index);
      }
      if (our_group) {
        BOOST_ASSERT(*our_group < session_info.validator_groups.size());
        if (grid_view) {
          BOOST_ASSERT(*our_group < grid_view->size());
        }
      }
      updatePeers(true);
    }

    ~PerSessionState() {
      updatePeers(false);
    }

    void updatePeers(bool add) const {
      if (not our_index or not our_group or not this->peers) {
        return;
      }
      auto &peers = *this->peers;
      SAFE_UNIQUE(peers) {
        auto f = [&](ValidatorIndex i) {
          auto &id = session_info.discovery_keys[i];
          auto it = peers.find(id);
          if (add) {
            if (it == peers.end()) {
              it = peers.emplace(id, 0).first;
            }
            ++it->second;
          } else {
            if (it == peers.end()) {
              throw std::logic_error{"inconsistent PeerUseCount"};
            }
            --it->second;
            if (it->second == 0) {
              peers.erase(it);
            }
          }
        };
        for (auto &i : session_info.validator_groups[*our_group]) {
          f(i);
        }
        if (grid_view) {
          auto &view = grid_view->at(*our_group);
          for (auto &i : view.sending) {
            f(i);
          }
          for (auto &i : view.receiving) {
            f(i);
          }
        }
      };
    }
  };

}  // namespace kagome::parachain::statement_distribution
