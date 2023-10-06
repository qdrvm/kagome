/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "common/visitor.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::approval {

  /// More tranches required - We're awaiting more assignments.
  struct PendingRequiredTranche {
    /// The highest considered delay tranche when counting assignments.
    DelayTranche considered;
    /// The tick at which the next no-show, of the assignments counted, would
    /// occur.
    std::optional<kagome::parachain::Tick> next_no_show;
    /// The highest tranche to consider when looking to broadcast own
    /// assignment. This should be considered along with the clock drift to
    /// avoid broadcasting assignments that are before the local time.
    DelayTranche maximum_broadcast;
    /// The clock drift, in ticks, to apply to the local clock when determining
    /// whether to broadcast an assignment or when to schedule a wakeup. The
    /// local clock should be treated as though it is `clock_drift` ticks
    /// earlier.
    kagome::parachain::Tick clock_drift;
  };

  struct ExactRequiredTranche {
    /// The tranche to inspect up to.
    DelayTranche needed;
    /// The amount of missing votes that should be tolerated.
    size_t tolerated_missing;
    /// When the next no-show would be, if any. This is used to schedule the
    /// next wakeup in the event that there are some assignments that don't have
    /// corresponding approval votes. If this is `None`, all assignments have
    /// approvals.
    std::optional<kagome::parachain::Tick> next_no_show;
    /// The last tick at which a needed assignment was received.
    std::optional<kagome::parachain::Tick> last_assignment_tick;
  };

  /// All validators appear to be required, based on tranches already taken and
  /// remaining no-shows.
  struct AllRequiredTranche {};

  /// The required tranches of assignments needed to determine whether a
  /// candidate is approved.
  using RequiredTranches = boost::
      variant<AllRequiredTranche, PendingRequiredTranche, ExactRequiredTranche>;

  /// The candidate is unapproved.
  struct Unapproved {};
  /// The candidate is approved, with the given amount of no-shows,
  /// with the last counted assignment being received at the given
  /// tick.
  using Approved = std::pair<size_t, std::optional<Tick>>;
  /// The candidate is approved by one third of all validators.
  struct ApprovedOneThird {};

  /// The result of a check.
  using Check = boost::variant<Unapproved, Approved, ApprovedOneThird>;

  /// Whether the candidate is approved and all relevant assignments
  /// have at most the given assignment tick.
  inline bool is_approved(const Check &self, const Tick max_assignment_tick) {
    if (is_type<Unapproved>(self)) {
      return false;
    }
    if (auto v{boost::get<Approved>(&self)}) {
      return (v->second ? (*v->second <= max_assignment_tick) : true);
    }
    if (is_type<ApprovedOneThird>(self)) {
      return true;
    }
    UNREACHABLE;
  }

  /// The number of known no-shows in this computation.
  inline size_t known_no_shows(const Check &self) {
    if (auto v{boost::get<Approved>(&self)}) {
      return v->first;
    }
    return 0ul;
  }

  inline size_t count_ones(const scale::BitVec &src) {
    size_t count_ones = 0ull;
    for (const auto v : src.bits) {
      count_ones += (v ? 1ull : 0ull);
    }
    return count_ones;
  }

  inline auto min_or_some(const std::optional<Tick> &l,
                          const std::optional<Tick> &r) {
    return (l && r) ? std::min(*l, *r)
         : l        ? *l
         : r        ? *r
                    : std::optional<Tick>{};
  };

  inline auto max_or_some(const std::optional<Tick> &l,
                          const std::optional<Tick> &r) {
    return (l && r) ? std::max(*l, *r)
         : l        ? *l
         : r        ? *r
                    : std::optional<Tick>{};
  };

  // Determining the amount of tranches required for approval or which
  // assignments are pending involves moving through a series of states while
  // looping over the tranches
  //
  // that we are aware of. First, we perform an initial count of the number of
  // assignments until we reach the number of needed assignments for approval.
  // As we progress, we count the number of no-shows in each tranche.
  //
  // Then, if there are any no-shows, we proceed into a series of subsequent
  // states for covering no-shows.
  //
  // We cover each no-show by a non-empty tranche, keeping track of the amount
  // of further no-shows encountered along the way. Once all of the no-shows we
  // were previously aware of are covered, we then progress to cover the
  // no-shows we encountered while covering those, and so on.
  struct State {
    /// The total number of assignments obtained.
    size_t assignments = 0ull;
    /// The depth of no-shows we are currently covering.
    size_t depth = 0ull;
    /// The amount of no-shows that have been covered at the previous or current
    /// depths.
    size_t covered = 0ull;
    /// The amount of assignments that we are attempting to cover at this depth.
    ///
    /// At depth 0, these are the initial needed approvals, and at other depths
    /// these are no-shows.
    size_t covering;
    /// The number of uncovered no-shows encountered at this depth. These will
    /// be the `covering` of the next depth.
    size_t uncovered = 0ull;
    /// The next tick at which a no-show would occur, if any.
    std::optional<Tick> next_no_show = std::nullopt;
    /// The last tick at which a considered assignment was received.
    std::optional<Tick> last_assignment_tick = std::nullopt;

    State(size_t covering_) : covering{covering_} {}
    State(size_t a,
          size_t d,
          size_t c,
          size_t cv,
          size_t ucvd,
          std::optional<Tick> nns,
          std::optional<Tick> lat)
        : assignments(a),
          depth(d),
          covered(c),
          covering(cv),
          uncovered(ucvd),
          next_no_show(nns),
          last_assignment_tick(lat) {}

    RequiredTranches output(DelayTranche tranche,
                            size_t needed_approvals,
                            size_t n_validators,
                            Tick no_show_duration) {
      const auto cv = (depth == 0ull) ? 0ull : covering;
      if (depth != 0 && (assignments + cv + uncovered >= n_validators)) {
        return AllRequiredTranche{};
      }

      if (assignments >= needed_approvals && (cv + uncovered) == 0) {
        return ExactRequiredTranche{
            .needed = tranche,
            .tolerated_missing = covered,
            .next_no_show = next_no_show,
            .last_assignment_tick = last_assignment_tick,
        };
      }

      const auto clock_drift = Tick(depth) * no_show_duration;
      if (depth == 0ull) {
        return PendingRequiredTranche{
            .considered = tranche,
            .next_no_show = next_no_show,
            .maximum_broadcast = std::numeric_limits<DelayTranche>::max(),
            .clock_drift = clock_drift,
        };
      } else {
        return PendingRequiredTranche{
            .considered = tranche,
            .next_no_show = next_no_show,
            .maximum_broadcast = tranche + DelayTranche(cv + uncovered),
            .clock_drift = clock_drift,
        };
      }
    }

    State advance(size_t new_assignments,
                  size_t new_no_shows,
                  const std::optional<Tick> &next_no_show_,
                  const std::optional<Tick> &last_assignment_tick_) {
      const auto new_covered = (depth == 0)
                                 ? new_assignments
                                 : std::min(new_assignments, size_t{1ull});
      const auto a = assignments + new_assignments;
      const auto c = math::sat_sub_unsigned(covering, new_covered);
      const auto cd = (depth == 0) ? 0ull : covered + new_covered;

      const auto ucd = uncovered + new_no_shows;
      const auto nns = min_or_some(next_no_show, next_no_show_);
      const auto lat = max_or_some(last_assignment_tick, last_assignment_tick_);

      size_t d, cv, ucvd;
      if (c != 0ull) {
        d = depth;
        cv = c;
        ucvd = ucd;
      } else {
        if (ucd == 0ull) {
          d = depth;
          cv = 0ull;
          ucvd = ucd;
        } else {
          d = depth + 1ull;
          cv = ucd;
          ucvd = 0ull;
        }
      }

      return State{a, d, cd, cv, ucvd, nns, lat};
    }
  };

}  // namespace kagome::parachain::approval
