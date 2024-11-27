/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>

#include <qtils/enum_error_code.hpp>

#include "parachain/types.hpp"

namespace kagome::parachain {
  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L435
  /// Separator between `XCM` and `UMPSignal`.
  inline const Buffer kUmpSeparator;

  enum class UmpError : uint8_t {
    TooManyUMPSignals,
  };
  Q_ENUM_ERROR_CODE(UmpError) {
    using E = decltype(e);
    switch (e) {
      case E::TooManyUMPSignals:
        return "Too many UMP signals";
    }
    abort();
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L432
  /// A message sent by a parachain to select the core the candidate is
  /// committed to. Relay chain validators, in particular backers, use the
  /// `CoreSelector` and `ClaimQueueOffset` to compute the index of the core the
  /// candidate has committed to.
  struct UMPSignalSelectCore {
    SCALE_TIE(2);

    uint8_t core_selector;
    uint8_t claim_queue_offset;
  };
  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L428
  /// Signals that a parachain can send to the relay chain via the UMP queue.
  using UMPSignal = boost::variant<UMPSignalSelectCore>;

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L438
  /// Utility function for skipping the ump signals.
  inline auto skipUmpSignals(std::span<const Buffer> messages) {
    return messages.first(std::ranges::find(messages, kUmpSeparator)
                          - messages.begin());
  }

  // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/primitives/src/vstaging/mod.rs#L447
  /// Returns the core selector and claim queue offset determined by
  /// `UMPSignal::SelectCore` commitment, if present.
  inline outcome::result<std::optional<UMPSignalSelectCore>> coreSelector(
      const network::CandidateCommitments &commitments) {
    auto it = std::ranges::find(commitments.upward_msgs, kUmpSeparator);
    if (it == commitments.upward_msgs.end()) {
      return std::nullopt;
    }
    ++it;
    if (it == commitments.upward_msgs.end()) {
      return std::nullopt;
    }
    OUTCOME_TRY(signal, scale::decode<UMPSignal>(*it));
    ++it;
    if (it != commitments.upward_msgs.end()) {
      return UmpError::TooManyUMPSignals;
    }
    return boost::get<UMPSignalSelectCore>(signal);
  }
}  // namespace kagome::parachain
