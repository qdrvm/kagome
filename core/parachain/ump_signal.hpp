/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>

#include <qtils/enum_error_code.hpp>

#include "common/buffer.hpp"
#include "scale/tie.hpp"

namespace kagome::parachain {
  /// Separator between `XCM` and `UMPSignal`.
  inline const Buffer kUmpSeparator;

  enum class UmpError {
    TOO_MANY_UMP_SIGNALS,
  };
  Q_ENUM_ERROR_CODE(UmpError) {
    using E = decltype(e);
    switch (e) {
      case E::TOO_MANY_UMP_SIGNALS:
        return "Too many UMP signals";
    }
    abort();
  }

  /// A message sent by a parachain to select the core the candidate is
  /// committed to. Relay chain validators, in particular backers, use the
  /// `CoreSelector` and `ClaimQueueOffset` to compute the index of the core the
  /// candidate has committed to.
  struct UMPSignalSelectCore {
    SCALE_TIE(2);

    uint8_t core_selector;
    uint8_t claim_queue_offset;
  };
  /// Signals that a parachain can send to the relay chain via the UMP queue.
  using UMPSignal = boost::variant<UMPSignalSelectCore>;

  /// Utility function for skipping the ump signals.
  inline auto skipUmpSignals(std::span<const Buffer> messages) {
    return messages.first(std::ranges::find(messages, kUmpSeparator)
                          - messages.begin());
  }

  /// Returns the core selector and claim queue offset determined by
  /// `UMPSignal::SelectCore` commitment, if present.
  inline outcome::result<std::optional<UMPSignalSelectCore>> coreSelector(
      const network::CandidateCommitments &receipt) {
    auto it = std::ranges::find(receipt.upward_msgs, kUmpSeparator);
    if (it == receipt.upward_msgs.end()) {
      return std::nullopt;
    }
    ++it;
    if (it == receipt.upward_msgs.end()) {
      return std::nullopt;
    }
    OUTCOME_TRY(signal, scale::decode<UMPSignal>(*it));
    ++it;
    if (it != receipt.upward_msgs.end()) {
      return UmpError::TOO_MANY_UMP_SIGNALS;
    }
    return boost::get<UMPSignalSelectCore>(signal);
  }
}  // namespace kagome::parachain
